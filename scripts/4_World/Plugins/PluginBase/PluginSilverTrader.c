// SilverBarter Haupt-Plugin (Server + Client vereint)
class PluginSilverTrader extends PluginBase
{
	// Werkzeug-Klassen fuer Kategorie-Filter (Client + Server)
	static ref array<string> TOOL_CLASSES;

	// Client-Seite
	ref SilverTraderMenu m_traderMenu;

	// Server-Seite
	ref SilverBarterConfig m_config;
	ref SilverRotatingTradersConfig m_rotatingConfig;
	ref map<int, TraderPoint> m_traderPoints;
	ref map<int, ref SilverTrader_ServerConfig> m_traderCache;
	ref map<int, ref SilverTrader_Data> m_traderData;
	ref set<int> m_dirtyTraders;
	float m_updateTimer = 0;
	float m_saveTimer = 0;
	const float SAVE_INTERVAL = 300.0; // Alle 5 Minuten speichern

	// Rotierende Haendler (Runtime-Daten, nicht persistent)
	ref map<int, TraderPoint> m_rotatingTraderPoints;
	ref map<int, ref SilverRotatingTrader_Config> m_rotatingTraderCache;
	ref map<int, ref SilverTrader_Data> m_rotatingTraderData;
	ref map<int, float> m_rotationTimers;
	bool m_zenMapMarkersSet = false;

	private const string DATA_FOLDER = "$profile:\\SilverBarter\\TraderData\\";

	override void OnInit()
	{
		// Werkzeug-Klassen initialisieren (Client + Server)
		if (!TOOL_CLASSES)
		{
			TOOL_CLASSES = new array<string>;
			TOOL_CLASSES.Insert("Hatchet");
			TOOL_CLASSES.Insert("Sickle");
			TOOL_CLASSES.Insert("Blowtorch");
			TOOL_CLASSES.Insert("Whetstone");
			TOOL_CLASSES.Insert("ElectronicRepairKit");
			TOOL_CLASSES.Insert("TireRepairKit");
			TOOL_CLASSES.Insert("LugWrench");
			TOOL_CLASSES.Insert("PipeWrench");
			TOOL_CLASSES.Insert("Screwdriver");
			TOOL_CLASSES.Insert("Hacksaw");
			TOOL_CLASSES.Insert("HandSaw");
			TOOL_CLASSES.Insert("Pliers");
			TOOL_CLASSES.Insert("Hammer");
			TOOL_CLASSES.Insert("CanOpener");
			TOOL_CLASSES.Insert("SewingKit");
			TOOL_CLASSES.Insert("LeatherSewingKit");
			TOOL_CLASSES.Insert("Lockpick");
		}

		// RPC-Handler registrieren (Client + Server)
		SilverRPCManager.RegisterHandler(SilverRPC.SILVERRPC_OPEN_TRADE_MENU, this, "RpcRequestOpen");
		SilverRPCManager.RegisterHandler(SilverRPC.SILVERRPC_ACTION_TRADER, this, "RpcHandleTraderAction");
		SilverRPCManager.RegisterHandler(SilverRPC.SILVERRPC_CLOSE_TRADER_MENU, this, "RpcRequestTraderMenuClose");
		SilverRPCManager.RegisterHandler(SilverRPC.SILVERRPC_ROTATING_TRADER_SYNC, this, "RpcRotatingTraderSync");

		// Server-Initialisierung
		if (g_Game.IsServer())
		{
			m_traderPoints = new map<int, TraderPoint>;
			m_traderCache = new map<int, ref SilverTrader_ServerConfig>;
			m_traderData = new map<int, ref SilverTrader_Data>;
			m_dirtyTraders = new set<int>;

			m_rotatingTraderPoints = new map<int, TraderPoint>;
			m_rotatingTraderCache = new map<int, ref SilverRotatingTrader_Config>;
			m_rotatingTraderData = new map<int, ref SilverTrader_Data>;
			m_rotationTimers = new map<int, float>;

			m_config = GetSilverBarterConfig();
			m_rotatingConfig = GetSilverRotatingTradersConfig();
		}
	}

	// ========== CLIENT-SEITE ==========

	void CloseTraderMenu()
	{
		if (m_traderMenu && m_traderMenu.m_active)
		{
			m_traderMenu.m_active = false;
		}
	}

	// Client: RPC empfangen - Menu oeffnen
	void RpcRequestOpen(ParamsReadContext ctx, PlayerIdentity sender)
	{
		if (!g_Game.IsClient())
			return;

		if (m_traderMenu && m_traderMenu.m_active)
		{
			m_traderMenu.m_active = false;
		}

		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!player)
			return;

		if (g_Game.GetUIManager().GetMenu() != null)
			return;

		// Daten einzeln lesen
		SilverTrader_Info traderInfo = new SilverTrader_Info();
		SilverTrader_Data traderData = new SilverTrader_Data();

		if (!ctx.Read(traderInfo.m_traderId)) return;
		if (!ctx.Read(traderInfo.m_position)) return;
		if (!ctx.Read(traderInfo.m_storageMaxSize)) return;
		if (!ctx.Read(traderInfo.m_storageCommission)) return;
		if (!ctx.Read(traderInfo.m_dumpingByAmountAlgorithm)) return;
		if (!ctx.Read(traderInfo.m_dumpingByAmountModifier)) return;
		if (!ctx.Read(traderInfo.m_dumpingByBadQuality)) return;
		if (!ctx.Read(traderInfo.m_sellMaxQuantityPercent)) return;
		if (!ctx.Read(traderInfo.m_buyMaxQuantityPercent)) return;

		bool isRotating;
		if (!ctx.Read(isRotating)) return;

		// Filter lesen
		int buyFilterCount;
		if (!ctx.Read(buyFilterCount)) return;
		traderInfo.m_buyFilter = new array<string>;
		for (int i = 0; i < buyFilterCount; i++)
		{
			string bf;
			if (!ctx.Read(bf)) return;
			traderInfo.m_buyFilter.Insert(bf);
		}

		int sellFilterCount;
		if (!ctx.Read(sellFilterCount)) return;
		traderInfo.m_sellFilter = new array<string>;
		for (int j = 0; j < sellFilterCount; j++)
		{
			string sf;
			if (!ctx.Read(sf)) return;
			traderInfo.m_sellFilter.Insert(sf);
		}

		// Commission-Overrides lesen
		int overrideCount;
		if (!ctx.Read(overrideCount)) return;
		traderInfo.m_commissionOverrides = new array<ref SilverCommissionOverride>;
		for (int ov = 0; ov < overrideCount; ov++)
		{
			string ovClass;
			float ovComm;
			if (!ctx.Read(ovClass)) return;
			if (!ctx.Read(ovComm)) return;
			SilverCommissionOverride ovEntry = new SilverCommissionOverride();
			ovEntry.classname = ovClass;
			ovEntry.commission = ovComm;
			traderInfo.m_commissionOverrides.Insert(ovEntry);
		}

		// Items lesen
		int itemCount;
		if (!ctx.Read(itemCount)) return;
		traderData.m_items = new map<string, float>;
		for (int k = 0; k < itemCount; k++)
		{
			string itemClass;
			float itemQty;
			if (!ctx.Read(itemClass)) return;
			if (!ctx.Read(itemQty)) return;
			traderData.m_items.Insert(itemClass, itemQty);
		}

		m_traderMenu = new SilverTraderMenu;
		m_traderMenu.InitMetadata(traderInfo.m_traderId, traderInfo, traderData, isRotating);
		g_Game.GetUIManager().ShowScriptedMenu(m_traderMenu, null);
	}

	// Client: RPC empfangen - Trade-Antwort
	void RpcHandleTraderAction(ParamsReadContext ctx, PlayerIdentity sender)
	{
		// Server-Handler
		if (g_Game.IsServer())
		{
			RpcRequestTraderAction(ctx, sender);
			return;
		}

		// Client-Handler: Update Trader-Daten nach Trade
		bool success;
		if (!ctx.Read(success))
			return;

		if (!success)
			return;

		// Items einzeln lesen
		int itemCount;
		if (!ctx.Read(itemCount))
			return;

		SilverTrader_Data newData = new SilverTrader_Data();
		for (int i = 0; i < itemCount; i++)
		{
			string itemClass;
			float itemQty;
			if (!ctx.Read(itemClass) || !ctx.Read(itemQty))
				return;
			newData.m_items.Insert(itemClass, itemQty);
		}

		if (m_traderMenu && m_traderMenu.m_active)
		{
			m_traderMenu.UpdateMetadata(newData);
		}
	}

	// ========== SERVER-SEITE ==========

	void InitializeTraders()
	{
		if (!g_Game.IsServer())
			return;

		if (!m_config || !m_config.m_traders)
		{
			Print("[SilverBarter] ERROR: Config not loaded!");
			return;
		}

		// Data-Ordner erstellen
		if (!FileExist(DATA_FOLDER))
		{
			MakeDirectory(DATA_FOLDER);
		}

		foreach (SilverTrader_ServerConfig trader : m_config.m_traders)
		{
			SpawnTrader(trader);
		}

		DebugLog(m_config.m_traders.Count().ToString() + " Trader initialisiert.");

		// Rotierende Haendler initialisieren
		if (m_rotatingConfig.m_rotatingTraders && m_rotatingConfig.m_rotatingTraders.Count() > 0)
		{
			foreach (SilverRotatingTrader_Config rotTrader : m_rotatingConfig.m_rotatingTraders)
			{
				SpawnRotatingTrader(rotTrader);
			}
			DebugLog(m_rotatingConfig.m_rotatingTraders.Count().ToString() + " Rotating Trader initialisiert.");
		}
	}

	private void SpawnTrader(SilverTrader_ServerConfig trader)
	{
		if (!trader || trader.m_traderId < 0)
		{
			Print("[SilverBarter] ERROR: Trader config invalid.");
			return;
		}

		// Trader-Daten aus JSON laden oder Default-Items verwenden
		string dataPath = DATA_FOLDER + "trader_" + trader.m_traderId.ToString() + ".json";
		SilverTrader_Data traderData = new SilverTrader_Data();

		if (FileExist(dataPath))
		{
			traderData.LoadFromJson(dataPath);
			if (!traderData.m_items)
				traderData.m_items = new map<string, float>;
		}
		else if (trader.m_defaultItems && trader.m_defaultItems.Count() > 0)
		{
			// Default-Items laden
			traderData.m_items = new map<string, float>;
			foreach (SilverTrader_ItemEntry defaultItem : trader.m_defaultItems)
			{
				if (defaultItem)
				{
					traderData.m_items.Insert(defaultItem.classname, defaultItem.quantity);
				}
			}
			traderData.SaveToJson(dataPath);
			DebugLog("Default items for trader " + trader.m_traderId.ToString() + " loaded.");
		}

		// Limitierte Items bei jedem Restart auf maxQuantity zuruecksetzen
		if (trader.m_limitedItems && trader.m_limitedItems.Count() > 0)
		{
			bool limitedChanged = false;
			foreach (SilverTrader_LimitedItem limitedItem : trader.m_limitedItems)
			{
				if (limitedItem && limitedItem.classname != "")
				{
					if (traderData.m_items.Contains(limitedItem.classname))
					{
						traderData.m_items.Set(limitedItem.classname, limitedItem.maxQuantity);
					}
					else
					{
						traderData.m_items.Insert(limitedItem.classname, limitedItem.maxQuantity);
					}
					limitedChanged = true;
				}
			}
			if (limitedChanged)
			{
				traderData.SaveToJson(dataPath);
				DebugLog("Limited items for trader " + trader.m_traderId.ToString() + " reset.");
			}
		}

		m_traderData.Insert(trader.m_traderId, traderData);

		// Trader-NPC spawnen
		Object traderObj = g_Game.CreateObject(trader.m_classname, trader.m_position);
		if (!traderObj)
		{
			Print("[SilverBarter] ERROR: Could not spawn trader NPC: " + trader.m_classname);
			return;
		}

		traderObj.SetAllowDamage(false);
		traderObj.SetPosition(trader.m_position);
		traderObj.SetOrientation(Vector(trader.m_orientation, 0, 0));

		// Attachments hinzufuegen
		EntityAI traderEntity;
		if (EntityAI.CastTo(traderEntity, traderObj) && trader.m_attachments)
		{
			foreach (string attachment : trader.m_attachments)
			{
				traderEntity.GetInventory().CreateInInventory(attachment);
			}
		}

		// TraderPoint erstellen (fuer Interaktion)
		TraderPoint traderPoint = TraderPoint.Cast(g_Game.CreateObject("TraderPoint", trader.m_position));
		if (traderPoint)
		{
			traderPoint.SetAllowDamage(false);
			traderPoint.SetPosition(trader.m_position);
			traderPoint.SetOrientation(Vector(trader.m_orientation, 0, 0));
			traderPoint.InitTraderPoint(trader.m_traderId, traderObj);
		}

		m_traderPoints.Insert(trader.m_traderId, traderPoint);
		m_traderCache.Insert(trader.m_traderId, trader);

		DebugLog("Trader " + trader.m_traderId.ToString() + " spawned.");
	}

	// ========== ROTIERENDE HAENDLER ==========

	private void SpawnRotatingTrader(SilverRotatingTrader_Config trader)
	{
		if (!trader || trader.m_traderId < 0)
		{
			Print("[SilverBarter] ERROR: Rotating trader config invalid.");
			return;
		}

		if (!trader.m_spawnPositions || trader.m_spawnPositions.Count() == 0)
		{
			Print("[SilverBarter] ERROR: Rotating trader has no positions configured.");
			return;
		}

		// Zufaellige Position waehlen
		int posIndex = Math.RandomInt(0, trader.m_spawnPositions.Count());
		vector spawnPos = trader.m_spawnPositions.Get(posIndex).ToVector();
		trader.m_position = spawnPos;

		DebugLog("Rotating Trader " + trader.m_traderId.ToString() + " spawns at position index " + posIndex.ToString());

		// Runtime-Daten erstellen und erste Rotation durchfuehren
		SilverTrader_Data traderData = new SilverTrader_Data();
		RotateTraderPool(trader, traderData);
		m_rotatingTraderData.Insert(trader.m_traderId, traderData);

		// NPC spawnen
		Object traderObj = g_Game.CreateObject(trader.m_classname, spawnPos);
		if (!traderObj)
		{
			Print("[SilverBarter] ERROR: Could not spawn rotating trader NPC: " + trader.m_classname);
			return;
		}

		traderObj.SetAllowDamage(false);
		traderObj.SetPosition(spawnPos);
		traderObj.SetOrientation(Vector(trader.m_orientation, 0, 0));

		// Attachments
		EntityAI traderEntity;
		if (EntityAI.CastTo(traderEntity, traderObj) && trader.m_attachments)
		{
			foreach (string attachment : trader.m_attachments)
			{
				traderEntity.GetInventory().CreateInInventory(attachment);
			}
		}

		// TraderPoint erstellen
		TraderPoint traderPoint = TraderPoint.Cast(g_Game.CreateObject("TraderPoint", spawnPos));
		if (traderPoint)
		{
			traderPoint.SetAllowDamage(false);
			traderPoint.SetPosition(spawnPos);
			traderPoint.SetOrientation(Vector(trader.m_orientation, 0, 0));
			traderPoint.InitTraderPoint(trader.m_traderId, traderObj, true);
		}

		m_rotatingTraderPoints.Insert(trader.m_traderId, traderPoint);
		m_rotatingTraderCache.Insert(trader.m_traderId, trader);
		m_rotationTimers.Insert(trader.m_traderId, 0);

		DebugLog("Rotating Trader " + trader.m_traderId.ToString() + " spawned with " + traderData.m_items.Count().ToString() + " items.");
	}

	private void RotateTraderPool(SilverRotatingTrader_Config trader, SilverTrader_Data traderData)
	{
		if (!trader || !trader.m_poolItems || trader.m_poolItems.Count() == 0)
			return;

		if (!traderData.m_items)
			traderData.m_items = new map<string, float>;
		else
			traderData.m_items.Clear();

		int slotsToFill = Math.Min(trader.m_activeSlots, trader.m_poolItems.Count());

		// Kopie der Pool-Indizes erstellen fuer gewichtete Auswahl ohne Zuruecklegen
		array<int> availableIndices = new array<int>;
		array<float> availableWeights = new array<float>;
		float totalWeight = 0;

		for (int i = 0; i < trader.m_poolItems.Count(); i++)
		{
			SilverTrader_PoolItem poolItem = trader.m_poolItems.Get(i);
			if (poolItem && poolItem.weight > 0)
			{
				availableIndices.Insert(i);
				availableWeights.Insert(poolItem.weight);
				totalWeight = totalWeight + poolItem.weight;
			}
		}

		for (int s = 0; s < slotsToFill; s++)
		{
			if (availableIndices.Count() == 0 || totalWeight <= 0)
				break;

			// Gewichtete Zufallsauswahl
			float roll = Math.RandomFloat(0, totalWeight);
			float cumulative = 0;
			int selectedIdx = -1;

			for (int w = 0; w < availableWeights.Count(); w++)
			{
				cumulative = cumulative + availableWeights.Get(w);
				if (roll <= cumulative)
				{
					selectedIdx = w;
					break;
				}
			}

			// Fallback: letztes Element
			if (selectedIdx == -1)
			{
				selectedIdx = availableWeights.Count() - 1;
			}

			int poolIndex = availableIndices.Get(selectedIdx);
			SilverTrader_PoolItem selected = trader.m_poolItems.Get(poolIndex);
			if (selected)
			{
				traderData.m_items.Insert(selected.classname, selected.quantity);
				DebugLog("Rotation: Selected " + selected.classname + " (qty: " + selected.quantity.ToString() + ", weight: " + selected.weight.ToString() + ")");
			}

			// Aus verfuegbaren Optionen entfernen
			totalWeight = totalWeight - availableWeights.Get(selectedIdx);
			availableIndices.Remove(selectedIdx);
			availableWeights.Remove(selectedIdx);
		}
	}

	private void CheckRotationTimers(float delta_time)
	{
		if (!m_rotatingTraderCache || !m_rotationTimers)
			return;

		foreach (int traderId, SilverRotatingTrader_Config config : m_rotatingTraderCache)
		{
			if (!config || config.m_rotationIntervalMinutes <= 0)
				continue;

			float timer = 0;
			if (m_rotationTimers.Contains(traderId))
			{
				timer = m_rotationTimers.Get(traderId);
			}

			timer = timer + delta_time;
			float intervalSeconds = config.m_rotationIntervalMinutes * 60.0;

			if (timer >= intervalSeconds)
			{
				timer = 0;

				SilverTrader_Data traderData;
				if (m_rotatingTraderData.Find(traderId, traderData))
				{
					RotateTraderPool(config, traderData);
					SyncRotatingTraderToClients(traderId);
					DebugLog("Rotating Trader " + traderId.ToString() + " pool rotated.");
				}
			}

			m_rotationTimers.Set(traderId, timer);
		}
	}

	private void SyncRotatingTraderToClients(int traderId)
	{
		SilverTrader_Data traderData;
		if (!m_rotatingTraderData.Find(traderId, traderData))
			return;

		// An alle verbundenen Spieler senden
		array<Man> players = new array<Man>;
		g_Game.GetPlayers(players);

		foreach (Man man : players)
		{
			PlayerBase player = PlayerBase.Cast(man);
			if (!player || !player.GetIdentity())
				continue;

			ScriptRPC rpc = new ScriptRPC();
			rpc.Write(SilverRPC.SILVERRPC_ROTATING_TRADER_SYNC);
			rpc.Write(traderId);

			int itemCount = 0;
			if (traderData && traderData.m_items)
				itemCount = traderData.m_items.Count();

			rpc.Write(itemCount);
			if (traderData && traderData.m_items)
			{
				for (int k = 0; k < traderData.m_items.Count(); k++)
				{
					rpc.Write(traderData.m_items.GetKey(k));
					rpc.Write(traderData.m_items.GetElement(k));
				}
			}

			rpc.Send(player, SilverRPCManager.CHANNEL_SILVER_BARTER, true, player.GetIdentity());
		}
	}

	// Client: Empfaengt neues Rotating-Trader Inventar nach Rotation
	void RpcRotatingTraderSync(ParamsReadContext ctx, PlayerIdentity sender)
	{
		if (!g_Game.IsClient())
			return;

		int traderId;
		if (!ctx.Read(traderId)) return;

		int itemCount;
		if (!ctx.Read(itemCount)) return;

		// Wenn Spieler gerade diesen Trader offen hat, aktualisieren
		SilverTrader_Data newData = new SilverTrader_Data();
		for (int i = 0; i < itemCount; i++)
		{
			string itemClass;
			float itemQty;
			if (!ctx.Read(itemClass) || !ctx.Read(itemQty))
				return;
			newData.m_items.Insert(itemClass, itemQty);
		}

		if (m_traderMenu && m_traderMenu.m_active && m_traderMenu.m_traderId == traderId)
		{
			m_traderMenu.UpdateMetadata(newData);
		}
	}

	// Prueft ob eine traderId zu einem rotierenden Haendler gehoert
	bool IsRotatingTrader(int traderId)
	{
		if (m_rotatingTraderCache && m_rotatingTraderCache.Contains(traderId))
			return true;
		return false;
	}

	#ifdef ZenMap
	private void SetZenMapMarker(SilverRotatingTrader_Config trader)
	{
		if (!trader || !trader.m_enableZenMapMarker)
			return;

		PluginZenMapMarkers zenMap = PluginZenMapMarkers.Cast(GetPlugin(PluginZenMapMarkers));
		if (!zenMap)
			return;

		string markerName = trader.m_zenMapMarkerName;
		if (markerName == "")
			markerName = "Trader " + trader.m_traderId.ToString();

		// Alten Marker entfernen falls vorhanden
		zenMap.RemoveMarkerByName(markerName);

		// Icon-Index ermitteln
		int iconIndex = 0;
		if (trader.m_zenMapMarkerIcon != "")
		{
			iconIndex = zenMap.GetIconIndex(trader.m_zenMapMarkerIcon);
		}

		// Neuen Marker erstellen
		MapMarker newMarker = new MapMarker(trader.m_position, markerName, ARGB(255, 255, 165, 0), iconIndex);
		if (newMarker)
		{
			zenMap.AddMarker(newMarker);
			DebugLog("ZenMap Marker gesetzt: " + markerName + " at " + trader.m_position.ToString());
		}
		else
		{
			Print("[SilverBarter] ERROR: MapMarker creation failed for " + markerName);
		}
	}

	private void RemoveZenMapMarker(SilverRotatingTrader_Config trader)
	{
		if (!trader || !trader.m_enableZenMapMarker)
			return;

		PluginZenMapMarkers zenMap = PluginZenMapMarkers.Cast(GetPlugin(PluginZenMapMarkers));
		if (!zenMap)
			return;

		string markerName = trader.m_zenMapMarkerName;
		if (markerName == "")
			markerName = "Trader " + trader.m_traderId.ToString();

		zenMap.RemoveMarkerByName(markerName);
		DebugLog("ZenMap Marker entfernt: " + markerName);
	}
	#endif

	void SendTraderMenuOpen(PlayerBase player, int traderId)
	{

		if (!g_Game.IsServer())
			return;

		if (!player || !player.GetIdentity())
			return;

		// Rotating Trader oder normaler Trader?
		SilverTrader_Info trader;
		SilverTrader_Data traderData;
		bool isRotating = IsRotatingTrader(traderId);

		if (isRotating)
		{
			SilverRotatingTrader_Config rotConfig;
			if (!m_rotatingTraderCache.Find(traderId, rotConfig))
				return;
			trader = rotConfig;
			if (!m_rotatingTraderData.Find(traderId, traderData))
				return;
		}
		else
		{
			SilverTrader_ServerConfig stdConfig;
			if (!m_traderCache.Find(traderId, stdConfig))
				return;
			trader = stdConfig;
			if (!m_traderData.Find(traderId, traderData))
				return;
		}

		TraderPoint traderPoint;
		if (isRotating)
		{
			if (!m_rotatingTraderPoints.Find(traderId, traderPoint))
				return;
		}
		else
		{
			if (!m_traderPoints.Find(traderId, traderPoint))
				return;
		}

		// RPC direkt senden mit einzelnen Werten (komplexe Objekte nicht serialisierbar)
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(SilverRPC.SILVERRPC_OPEN_TRADE_MENU);

		// Trader-Info einzeln schreiben
		rpc.Write(trader.m_traderId);
		rpc.Write(trader.m_position);
		rpc.Write(trader.m_storageMaxSize);
		rpc.Write(trader.m_storageCommission);
		rpc.Write(trader.m_dumpingByAmountAlgorithm);
		rpc.Write(trader.m_dumpingByAmountModifier);
		rpc.Write(trader.m_dumpingByBadQuality);
		rpc.Write(trader.m_sellMaxQuantityPercent);
		rpc.Write(trader.m_buyMaxQuantityPercent);
		rpc.Write(isRotating);

		// Filter als String-Arrays (mit Null-Check)
		int buyFilterCount = 0;
		if (trader.m_buyFilter)
		{
			buyFilterCount = trader.m_buyFilter.Count();
		}
		rpc.Write(buyFilterCount);
		for (int i = 0; i < buyFilterCount; i++)
		{
			rpc.Write(trader.m_buyFilter.Get(i));
		}

		int sellFilterCount = 0;
		if (trader.m_sellFilter)
		{
			sellFilterCount = trader.m_sellFilter.Count();
		}
		rpc.Write(sellFilterCount);
		for (int j = 0; j < sellFilterCount; j++)
		{
			rpc.Write(trader.m_sellFilter.Get(j));
		}

		// Commission-Overrides
		int overrideCount = 0;
		if (trader.m_commissionOverrides)
		{
			overrideCount = trader.m_commissionOverrides.Count();
		}
		rpc.Write(overrideCount);
		for (int ov = 0; ov < overrideCount; ov++)
		{
			rpc.Write(trader.m_commissionOverrides.Get(ov).classname);
			rpc.Write(trader.m_commissionOverrides.Get(ov).commission);
		}

		// Trader-Data (Items) als Key-Value Paare
		int itemCount = 0;
		if (traderData.m_items)
			itemCount = traderData.m_items.Count();
		rpc.Write(itemCount);
		for (int k = 0; k < itemCount; k++)
		{
			rpc.Write(traderData.m_items.GetKey(k));
			rpc.Write(traderData.m_items.GetElement(k));
		}

		rpc.Send(player, SilverRPCManager.CHANNEL_SILVER_BARTER, true, player.GetIdentity());
		DebugLog("Menu RPC sent to " + player.GetIdentity().GetName());
	}

	void RpcRequestTraderMenuClose(ParamsReadContext ctx, PlayerIdentity sender)
	{
		if (!g_Game.IsServer())
			return;

		int traderId;
		if (!ctx.Read(traderId))
		{
			Print("[SilverBarter] ERROR: Failed to read close traderId");
			return;
		}

	}

	void RpcRequestTraderAction(ParamsReadContext ctx, PlayerIdentity sender)
	{
		if (!g_Game.IsServer())
			return;

		PlayerBase player = GetPlayerByIdentity(sender);
		if (!player)
			return;

		DebugLog("Trade request from " + sender.GetName());

		// Daten einzeln lesen
		int traderId;
		if (!ctx.Read(traderId))
		{
			Print("[SilverBarter] ERROR: Failed to read traderId");
			return;
		}

		// Sell-Items als NetworkIDs lesen
		int sellCount;
		if (!ctx.Read(sellCount))
		{
			Print("[SilverBarter] ERROR: Failed to read sellCount");
			return;
		}
		if (sellCount < 0 || sellCount > 100)
		{
			Print("[SilverBarter] ERROR: sellCount out of limit: " + sellCount.ToString());
			return;
		}

		array<ItemBase> sellItems = new array<ItemBase>;
		for (int s = 0; s < sellCount; s++)
		{
			int lowBits, highBits;
			if (!ctx.Read(lowBits)) return;
			if (!ctx.Read(highBits)) return;

			Object obj = g_Game.GetObjectByNetworkId(lowBits, highBits);
			ItemBase item = ItemBase.Cast(obj);
			if (item && sellItems.Find(item) == -1)
			{
				sellItems.Insert(item);
			}
		}

		// Buy-Items lesen
		int buyCount;
		if (!ctx.Read(buyCount))
		{
			Print("[SilverBarter] ERROR: Failed to read buyCount");
			return;
		}
		if (buyCount < 0 || buyCount > 10)
		{
			Print("[SilverBarter] ERROR: buyCount out of limit: " + buyCount.ToString());
			return;
		}

		map<string, float> buyItems = new map<string, float>;
		for (int b = 0; b < buyCount; b++)
		{
			string buyClass;
			float buyQty;
			if (!ctx.Read(buyClass)) return;
			if (!ctx.Read(buyQty)) return;
			if (buyQty > 50)
				buyQty = 50;
			if (buyQty > 0)
			{
				if (buyItems.Contains(buyClass))
				{
					buyItems.Set(buyClass, Math.Min(50, buyItems.Get(buyClass) + buyQty));
				}
				else
				{
					buyItems.Insert(buyClass, buyQty);
				}
			}
		}

		DebugLog("Trade: " + sellItems.Count().ToString() + " selling, " + buyItems.Count().ToString() + " buying");

		bool isRotatingTrade = IsRotatingTrader(traderId);
		SilverTrader_Info traderInfo;
		SilverTrader_Data traderData;

		if (isRotatingTrade)
		{
			SilverRotatingTrader_Config rotConfig;
			if (!m_rotatingTraderCache.Find(traderId, rotConfig))
				return;
			traderInfo = rotConfig;
			if (!m_rotatingTraderData.Find(traderId, traderData))
				return;
		}
		else
		{
			SilverTrader_ServerConfig stdConfig;
			if (!m_traderCache.Find(traderId, stdConfig))
				return;
			traderInfo = stdConfig;
			if (!m_traderData.Find(traderId, traderData))
				return;
		}

		// Distanz-Check (max 5m zum Trader)
		float dist = vector.Distance(player.GetPosition(), traderInfo.m_position);
		if (dist > 5.0)
		{
			DebugLog("Trade denied: Player too far away (" + dist.ToString() + "m)");
			return;
		}

		// Barter-Regel: Verkauf nur mit Gegenkauf erlaubt (kein einseitiges Abgeben)
		if (sellItems.Count() > 0 && buyItems.Count() == 0)
		{
			DebugLog("Trade denied: Sell without counter-purchase from " + sender.GetName());
			return;
		}

		// === PHASE 1: Sell-Items validieren + SellMaxQuantity enforced ===
		array<ItemBase> validSellItems = new array<ItemBase>;
		map<string, float> sellCounter = new map<string, float>;
		foreach (ItemBase sellItem1 : sellItems)
		{
			if (!sellItem1)
				continue;
			if (!IsItemOwnedByPlayer(sellItem1, player))
				continue;
			if (!CanSellItem(traderInfo, sellItem1))
				continue;

			string sellClass = sellItem1.GetType();
			float sellQty01 = CalculateItemQuantity01(sellItem1);
			float currentSellCount = 0;
			if (sellCounter.Contains(sellClass))
				currentSellCount = sellCounter.Get(sellClass);

			float sellMax = CalculateSellMaxQuantity(traderInfo, sellClass);
			if (currentSellCount + sellQty01 > sellMax)
			{
				DebugLog("Trade denied: SellMaxQuantity exceeded for " + sellClass);
				continue;
			}

			// Storage-Kapazitaet pruefen (nur normale Trader)
			if (!isRotatingTrade)
			{
				float storedQty = 0;
				if (traderData.m_items.Contains(sellClass))
					storedQty = traderData.m_items.Get(sellClass);
				float storageMax = CalculateTraderItemQuantityMax(traderInfo, sellClass);
				if (storedQty + currentSellCount + sellQty01 > storageMax)
				{
					DebugLog("Trade denied: Storage full for " + sellClass);
					continue;
				}
			}

			sellCounter.Set(sellClass, currentSellCount + sellQty01);
			validSellItems.Insert(sellItem1);
		}

		// === PHASE 2: Buy-Items gegen Stock + BuyMaxQuantity validieren ===
		map<string, float> approvedBuyItems = new map<string, float>;
		foreach (string buyClassname1, float buyQuantity1 : buyItems)
		{
			if (!CanBuyItem(traderInfo, buyClassname1))
				continue;

			if (!traderData.m_items.Contains(buyClassname1))
			{
				DebugLog("Trade denied: Item not in stock: " + buyClassname1);
				continue;
			}

			// BuyMaxQuantity serverseitig enforced
			float buyMax = CalculateBuyMaxQuantity(traderInfo, buyClassname1);
			float clampedQty = Math.Min(buyQuantity1, buyMax);

			float availableStock = traderData.m_items.Get(buyClassname1);
			float approvedQty = Math.Min(clampedQty, availableStock);
			if (approvedQty <= 0)
			{
				DebugLog("Trade denied: Insufficient stock for " + buyClassname1);
				continue;
			}

			approvedBuyItems.Insert(buyClassname1, approvedQty);
		}

		// === PHASE 3: Preis nur fuer validierte Items berechnen ===
		int resultPrice = 0;

		foreach (ItemBase sellItem2 : validSellItems)
		{
			resultPrice = resultPrice + CalculateSellPrice(traderInfo, traderData, sellItem2);
		}

		foreach (string buyClassname2, float buyQuantity2 : approvedBuyItems)
		{
			resultPrice = resultPrice - CalculateBuyPrice(traderInfo, traderData, buyClassname2, buyQuantity2);
		}

		if (resultPrice < 0)
		{
			DebugLog("Trade denied: Negative price for " + sender.GetName());
			return;
		}

		// === PHASE 4: Trade ausfuehren ===
		// Sell-Items ins Trader-Inventar aufnehmen
		foreach (ItemBase sellItem3 : validSellItems)
		{
			string classname = sellItem3.GetType();

			if (isRotatingTrade)
			{
				DebugLog("Rotating Trader " + traderId.ToString() + " destroys sold item: " + classname);
				continue;
			}

			float maxQuantity = CalculateTraderItemQuantityMax(traderInfo, classname);
			float itemQuantity = CalculateItemQuantity01(sellItem3);
			float newValue = 0;

			if (traderData.m_items.Contains(classname))
			{
				newValue = Math.Min(maxQuantity, traderData.m_items.Get(classname) + itemQuantity);
				traderData.m_items.Set(classname, newValue);
			}
			else
			{
				newValue = Math.Min(maxQuantity, itemQuantity);
				traderData.m_items.Set(classname, newValue);
			}

			DebugLog("Trader " + traderId.ToString() + " buys: " + classname);
		}

		// Buy-Items: Bestand reduzieren
		foreach (string buyClassname3, float buyQuantity3 : approvedBuyItems)
		{
			float newValue2 = Math.Max(0, traderData.m_items.Get(buyClassname3) - buyQuantity3);
			if (newValue2 == 0)
			{
				traderData.m_items.Remove(buyClassname3);
			}
			else
			{
				traderData.m_items.Set(buyClassname3, newValue2);
			}
			DebugLog("Trader " + traderId.ToString() + " sells: " + buyClassname3);
		}

		// Verkaufte Items loeschen (rueckwaerts, Attachments vor Container)
		for (int i = validSellItems.Count() - 1; i >= 0; i--)
		{
			ItemBase sellItem4 = validSellItems[i];
			if (sellItem4 && !sellItem4.IsPendingDeletion() && IsItemOwnedByPlayer(sellItem4, player))
			{
				g_Game.ObjectDelete(sellItem4);
			}
		}

		// Gekaufte Items spawnen (nur freigegebene Items mit validierter Menge)
		foreach (string buyClassname4, float buyQuantity4 : approvedBuyItems)
		{

			if (buyQuantity4 <= 0)
			{
				DebugLog("SPAWN SKIP: buyQuantity is 0 for " + buyClassname4);
				continue;
			}

			float calcQuantity = buyQuantity4;
			while (calcQuantity > 0)
			{
				ItemBase buyEntity = null;
				InventoryLocation invLoc = new InventoryLocation;
				bool foundInvSlot = player.GetInventory().FindFirstFreeLocationForNewEntity(buyClassname4, FindInventoryLocationType.ANY, invLoc);

				if (foundInvSlot)
				{
					buyEntity = ItemBase.Cast(player.GetInventory().LocationCreateEntity(invLoc, buyClassname4, ECE_IN_INVENTORY, RF_DEFAULT));
				}
				else
				{
					buyEntity = ItemBase.Cast(g_Game.CreateObject(buyClassname4, player.GetPosition()));
				}

				if (buyEntity)
				{
					float spawnQuantity01 = Math.Clamp(calcQuantity, 0, 1);
					Magazine buyMagazine;

					if (buyEntity.IsInherited(Magazine))
					{
						Class.CastTo(buyMagazine, buyEntity);
						if (buyEntity.IsInherited(Ammunition_Base))
						{
							buyMagazine.ServerSetAmmoCount((int)Math.Round(buyMagazine.GetAmmoMax() * spawnQuantity01));
						}
						else
						{
							buyMagazine.ServerSetAmmoCount(0);
						}
					}
					else if (g_Game.ConfigIsExisting(CFG_VEHICLESPATH + " " + buyClassname4 + " liquidContainerType"))
					{
						buyEntity.SetQuantityNormalized(0);
					}
					else
					{
						buyEntity.SetQuantityNormalized(spawnQuantity01);
					}

					DebugLog("SPAWN OK: " + buyClassname4 + " (inv=" + foundInvSlot.ToString() + ", qty=" + spawnQuantity01.ToString() + ")");
				}
				else
				{
					Print("[SilverBarter] SPAWN FAILED: " + buyClassname4 + " could not be created (inv=" + foundInvSlot.ToString() + ")");
					break;
				}

				calcQuantity = calcQuantity - 1;
			}
		}

		// Pruefen ob tatsaechlich etwas getauscht wurde
		bool tradeSuccess = (validSellItems.Count() > 0 || approvedBuyItems.Count() > 0);

		if (!tradeSuccess)
		{
			DebugLog("Trade result: Nothing traded for " + sender.GetName());
		}

		// Trader als dirty markieren (nur normale Trader persistent speichern)
		if (tradeSuccess && !isRotatingTrade)
		{
			m_dirtyTraders.Insert(traderId);
		}

		// Antwort an Client senden
		PlayerBase respPlayer = GetPlayerByIdentity(sender);
		if (respPlayer)
		{
			ScriptRPC respRpc = new ScriptRPC();
			respRpc.Write(SilverRPC.SILVERRPC_ACTION_TRADER);
			respRpc.Write(tradeSuccess);

			int respItemCount = 0;
			if (traderData && traderData.m_items)
				respItemCount = traderData.m_items.Count();

			respRpc.Write(respItemCount);
			if (traderData && traderData.m_items)
			{
				for (int ri = 0; ri < traderData.m_items.Count(); ri++)
				{
					respRpc.Write(traderData.m_items.GetKey(ri));
					respRpc.Write(traderData.m_items.GetElement(ri));
				}
			}
			respRpc.Send(respPlayer, SilverRPCManager.CHANNEL_SILVER_BARTER, true, sender);
			DebugLog("Trade response sent: success=" + tradeSuccess.ToString() + ", items=" + respItemCount.ToString());
		}
	}

	void SaveTraderData(int traderId)
	{
		SilverTrader_Data traderData;
		if (m_traderData.Find(traderId, traderData))
		{
			string dataPath = DATA_FOLDER + "trader_" + traderId.ToString() + ".json";
			traderData.SaveToJson(dataPath);
		}
	}

	void SaveDirtyTraderData()
	{
		if (!g_Game.IsServer())
			return;

		if (!m_dirtyTraders || m_dirtyTraders.Count() == 0)
			return;

		foreach (int traderId : m_dirtyTraders)
		{
			SaveTraderData(traderId);
		}

		if (m_config && m_config.m_debugMode)
		{
			Print("[SilverBarter] " + m_dirtyTraders.Count().ToString() + " trader data saved.");
		}

		m_dirtyTraders.Clear();
	}

	void SaveAllTraderData()
	{
		if (!g_Game.IsServer())
			return;

		if (!m_traderData)
			return;

		foreach (int traderId, SilverTrader_Data data : m_traderData)
		{
			SaveTraderData(traderId);
		}

		if (m_dirtyTraders)
		{
			m_dirtyTraders.Clear();
		}

		Print("[SilverBarter] All trader data saved (shutdown).");
	}

	private bool IsItemOwnedByPlayer(ItemBase item, PlayerBase player)
	{
		if (!item || !player)
			return false;

		EntityAI root = item.GetHierarchyRoot();
		if (root == player)
			return true;

		return false;
	}

	private PlayerBase GetPlayerByIdentity(PlayerIdentity identity)
	{
		if (!identity)
			return null;

		int highBits, lowBits;
		g_Game.GetPlayerNetworkIDByIdentityID(identity.GetPlayerId(), lowBits, highBits);
		return PlayerBase.Cast(g_Game.GetObjectByNetworkId(lowBits, highBits));
	}

	override void OnUpdate(float delta_time)
	{
		if (!g_Game.IsServer())
			return;

		// Trader-Tick
		m_updateTimer = m_updateTimer + delta_time;
		if (m_updateTimer > 1.0)
		{
			m_updateTimer = 0;
			foreach (int traderId, TraderPoint point : m_traderPoints)
			{
				if (point)
				{
					point.OnTick();
				}
			}
			// Rotierende Trader ebenfalls ticken
			if (m_rotatingTraderPoints)
			{
				foreach (int rotTraderId, TraderPoint rotPoint : m_rotatingTraderPoints)
				{
					if (rotPoint)
					{
						rotPoint.OnTick();
					}
				}
			}
		}

		// Periodisches Speichern (nur dirty Trader)
		m_saveTimer = m_saveTimer + delta_time;
		if (m_saveTimer > SAVE_INTERVAL)
		{
			m_saveTimer = 0;
			SaveDirtyTraderData();
		}

		// Rotierende Haendler Timer pruefen
		CheckRotationTimers(delta_time);

		// ZenMap Marker verzoegert setzen (warten bis ZenMap-Plugin bereit ist)
		#ifdef ZenMap
		if (!m_zenMapMarkersSet && m_rotatingTraderCache)
		{
			PluginZenMapMarkers zenCheck = PluginZenMapMarkers.Cast(GetPlugin(PluginZenMapMarkers));
			if (zenCheck)
			{
				foreach (int zenId, SilverRotatingTrader_Config zenCfg : m_rotatingTraderCache)
				{
					SetZenMapMarker(zenCfg);
				}
				m_zenMapMarkersSet = true;
				DebugLog("ZenMap Marker fuer rotierende Haendler gesetzt.");
			}
		}
		#endif
	}

	override void OnDestroy()
	{
		// Beim Beenden alle Daten speichern
		SaveAllTraderData();

		if (m_traderPoints)
		{
			foreach (int id, TraderPoint obj : m_traderPoints)
			{
				if (obj)
				{
					g_Game.ObjectDelete(obj);
				}
			}
			delete m_traderPoints;
		}

		if (m_traderCache)
			delete m_traderCache;

		if (m_traderData)
			delete m_traderData;

		if (m_dirtyTraders)
			delete m_dirtyTraders;

		// ZenMap Marker entfernen
		#ifdef ZenMap
		if (m_rotatingTraderCache)
		{
			foreach (int zenTraderId, SilverRotatingTrader_Config zenConfig : m_rotatingTraderCache)
			{
				RemoveZenMapMarker(zenConfig);
			}
		}
		#endif

		// Rotierende Haendler bereinigen
		if (m_rotatingTraderPoints)
		{
			foreach (int rotId, TraderPoint rotObj : m_rotatingTraderPoints)
			{
				if (rotObj)
				{
					g_Game.ObjectDelete(rotObj);
				}
			}
			delete m_rotatingTraderPoints;
		}

		if (m_rotatingTraderCache)
			delete m_rotatingTraderCache;

		if (m_rotatingTraderData)
			delete m_rotatingTraderData;

		if (m_rotationTimers)
			delete m_rotationTimers;
	}

	// ========== BERECHNUNGS-FUNKTIONEN (Client + Server) ==========

	bool HasOversizedSellItems(SilverTrader_Info traderInfo, SilverTrader_Data data, map<string, float> sellCounter)
	{
		foreach (string classname, float quantity : sellCounter)
		{
			if (quantity > CalculateSellMaxQuantity(traderInfo, classname))
			{
				return true;
			}

			if (data.m_items.Contains(classname))
			{
				float storedQuantity = data.m_items.Get(classname);
				float maxQuantity = CalculateTraderItemQuantityMax(traderInfo, classname);
				if (storedQuantity + quantity > maxQuantity)
				{
					return true;
				}
			}
		}
		return false;
	}

	int CalculateSellPrice(SilverTrader_Info trader, SilverTrader_Data data, ItemBase item)
	{
		if (!item)
			return 0;

		int healthlevel = item.GetHealthLevel();
		if (healthlevel > GameConstants.STATE_WORN)
			return 0;

		string classname = item.GetType();
		float traderTotalQuantity = 0;
		if (data.m_items.Find(classname, traderTotalQuantity))
		{
			// Gefunden
		}

		float resultPrice = CalculateBuyPrice(trader, data, classname, 1);
		float commission = trader.GetCommissionForItem(classname);
		resultPrice = resultPrice - (commission * resultPrice);
		resultPrice = resultPrice * CalculateItemQuantity01(item);

		if (healthlevel == GameConstants.STATE_WORN)
		{
			resultPrice = resultPrice * trader.m_dumpingByBadQuality;
		}

		if (resultPrice < 0)
			return 0;

		return (int)Math.Floor(resultPrice);
	}

	int CalculateBuyPrice(SilverTrader_Info trader, SilverTrader_Data data, string classname, float quantity)
	{
		float totalQuantity = 0;
		if (data.m_items.Contains(classname))
		{
			totalQuantity = data.m_items.Get(classname);
		}

		float itemMaxQuantity = CalculateTraderItemQuantityMax(trader, classname);
		quantity = Math.Min(quantity, itemMaxQuantity);
		totalQuantity = Math.Min(totalQuantity, itemMaxQuantity);

		float resultPrice = CalculateDumping(trader.m_dumpingByAmountAlgorithm, trader.m_dumpingByAmountModifier, (int)totalQuantity, (int)itemMaxQuantity);
		resultPrice = resultPrice * quantity * 1000;

		if (resultPrice < 1)
			return 1;

		return (int)Math.Floor(resultPrice);
	}

	float CalculateTraderItemQuantityMax(SilverTrader_Info trader, string classname)
	{
		vector itemSize = "1 1 0";

		if (g_Game.ConfigIsExisting(CFG_VEHICLESPATH + " " + classname))
		{
			itemSize = g_Game.ConfigGetVector(CFG_VEHICLESPATH + " " + classname + " itemSize");
		}
		else if (g_Game.ConfigIsExisting(CFG_MAGAZINESPATH + " " + classname))
		{
			itemSize = g_Game.ConfigGetVector(CFG_MAGAZINESPATH + " " + classname + " itemSize");
		}
		else if (g_Game.ConfigIsExisting(CFG_WEAPONSPATH + " " + classname))
		{
			itemSize = g_Game.ConfigGetVector(CFG_WEAPONSPATH + " " + classname + " itemSize");
		}

		int itemCapacity = (int)Math.Max(1, itemSize[0] * itemSize[1]);
		return Math.Round(((float)trader.m_storageMaxSize) / itemCapacity);
	}

	float CalculateItemQuantity01(ItemBase item)
	{
		if (item.GetLiquidTypeInit() != 0 && !item.IsInherited(Bottle_Base))
			return 1;

		float item_quantity = item.GetQuantity();
		int max_quantity = item.GetQuantityMax();

		if (max_quantity > 0)
		{
			if (item.IsInherited(Ammunition_Base))
			{
				Magazine magazine_item;
				Class.CastTo(magazine_item, item);
				return (float)magazine_item.GetAmmoCount() / (float)magazine_item.GetAmmoMax();
			}
			else if (item.IsInherited(Magazine))
			{
				return 1;
			}
			else
			{
				return Math.Min(item_quantity, max_quantity) / (float)max_quantity;
			}
		}

		return 1;
	}

	float CalculateSellMaxQuantity(SilverTrader_Info traderInfo, string classname)
	{
		float result = CalculateTraderItemQuantityMax(traderInfo, classname) * traderInfo.m_sellMaxQuantityPercent;
		result = Math.Round(result);
		if (result < 1)
			result = 1;
		return result;
	}

	float CalculateBuyMaxQuantity(SilverTrader_Info traderInfo, string classname)
	{
		float result = CalculateTraderItemQuantityMax(traderInfo, classname) * traderInfo.m_buyMaxQuantityPercent;
		result = Math.Round(result);
		if (result < 1)
			result = 1;
		return result;
	}

	float CalculateItemSelectedQuantityStep(string classname)
	{
		if (g_Game.ConfigIsExisting(CFG_VEHICLESPATH + " " + classname + " liquidContainerType"))
			return 1;

		int maxStackSize = 0;

		if (g_Game.ConfigIsExisting(CFG_VEHICLESPATH + " " + classname))
		{
			maxStackSize = g_Game.ConfigGetInt(CFG_VEHICLESPATH + " " + classname + " varQuantityMax");
		}
		else if (g_Game.ConfigIsExisting(CFG_MAGAZINESPATH + " " + classname))
		{
			maxStackSize = g_Game.ConfigGetInt(CFG_MAGAZINESPATH + " " + classname + " count");
		}
		else if (g_Game.ConfigIsExisting(CFG_WEAPONSPATH + " " + classname))
		{
			maxStackSize = g_Game.ConfigGetInt(CFG_WEAPONSPATH + " " + classname + " varQuantityMax");
		}

		if (maxStackSize > 0)
		{
			string stackedUnits = "";

			if (g_Game.ConfigIsExisting(CFG_VEHICLESPATH + " " + classname))
			{
				stackedUnits = g_Game.ConfigGetTextOut(CFG_VEHICLESPATH + " " + classname + " stackedUnit");
			}
			else if (g_Game.ConfigIsExisting(CFG_MAGAZINESPATH + " " + classname))
			{
				stackedUnits = g_Game.ConfigGetTextOut(CFG_MAGAZINESPATH + " " + classname + " stackedUnit");
			}
			else if (g_Game.ConfigIsExisting(CFG_WEAPONSPATH + " " + classname))
			{
				stackedUnits = g_Game.ConfigGetTextOut(CFG_WEAPONSPATH + " " + classname + " stackedUnit");
			}

			if (stackedUnits == "pc.")
			{
				return 1.0 / maxStackSize;
			}
			else if (g_Game.IsKindOf(classname, "Ammunition_Base"))
			{
				return 1.0 / maxStackSize;
			}
		}

		return 1;
	}

	float CalculateDumping(string algorithm, float modifier, float value, float max)
	{
		return Math.Lerp(1, modifier, (value / max));
	}

	void DoBarter(int traderId, array<ItemBase> sellItems, map<string, float> buyItems)
	{
		if (sellItems.Count() == 0 && buyItems.Count() == 0)
			return;

		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!player)
			return;

		// Erst valide Items zaehlen
		int validSellCount = 0;
		foreach (ItemBase countItem : sellItems)
		{
			if (countItem)
				validSellCount++;
		}

		// RPC mit serialisierbaren Daten senden
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(SilverRPC.SILVERRPC_ACTION_TRADER);
		rpc.Write(traderId);

		// Sell-Items als NetworkIDs senden (nur valide)
		rpc.Write(validSellCount);
		foreach (ItemBase sellItem : sellItems)
		{
			if (!sellItem)
				continue;

			int lowBits, highBits;
			sellItem.GetNetworkID(lowBits, highBits);
			rpc.Write(lowBits);
			rpc.Write(highBits);
		}

		// Buy-Items als Key/Value Paare
		rpc.Write(buyItems.Count());
		for (int i = 0; i < buyItems.Count(); i++)
		{
			rpc.Write(buyItems.GetKey(i));
			rpc.Write(buyItems.GetElement(i));
		}

		rpc.Send(player, SilverRPCManager.CHANNEL_SILVER_BARTER, true);
	}

	private bool IsCategoryEnabled(array<string> categories, array<bool> enabledCategories, string categoryName)
	{
		int idx = categories.Find(categoryName);
		if (idx < 0 || idx >= enabledCategories.Count())
			return false;
		return enabledCategories.Get(idx);
	}

	bool FilterByCategories(array<string> categories, array<bool> enabledCategories, string classname)
	{
		TStringArray inventorySlots = new TStringArray;

		if (g_Game.IsKindOf(classname, "Grenade_Base") || g_Game.ConfigIsExisting(CFG_WEAPONSPATH + " " + classname))
		{
			return IsCategoryEnabled(categories, enabledCategories, "weapons");
		}

		if (g_Game.IsKindOf(classname, "Ammunition_Base") || classname.IndexOf("AmmoBox") == 0)
		{
			return IsCategoryEnabled(categories, enabledCategories, "ammo");
		}

		if (g_Game.IsKindOf(classname, "Magazine_Base"))
		{
			return IsCategoryEnabled(categories, enabledCategories, "magazines");
		}

		if (TOOL_CLASSES)
		{
			foreach (string toolClass : TOOL_CLASSES)
			{
				if (g_Game.IsKindOf(classname, toolClass))
				{
					return IsCategoryEnabled(categories, enabledCategories, "tools");
				}
			}
		}

		if (g_Game.ConfigIsExisting(CFG_VEHICLESPATH + " " + classname + " inventorySlot"))
		{
			inventorySlots.Clear();
			g_Game.ConfigGetTextArray(CFG_VEHICLESPATH + " " + classname + " inventorySlot", inventorySlots);

			foreach (string inventorySlot : inventorySlots)
			{
				inventorySlot.ToLower();
				if (inventorySlot.IndexOf("weapon") == 0)
				{
					return IsCategoryEnabled(categories, enabledCategories, "attachments");
				}
				else if (inventorySlot == "melee")
				{
					return IsCategoryEnabled(categories, enabledCategories, "tools");
				}
			}
		}

		if (g_Game.ConfigIsExisting(CFG_VEHICLESPATH + " " + classname + " attachments"))
		{
			inventorySlots.Clear();
			g_Game.ConfigGetTextArray(CFG_VEHICLESPATH + " " + classname + " attachments", inventorySlots);

			foreach (string attachment : inventorySlots)
			{
				attachment.ToLower();
				if (attachment.IndexOf("batteryd") == 0)
				{
					return IsCategoryEnabled(categories, enabledCategories, "electronic");
				}
			}
		}

		if (g_Game.ConfigGetInt(CFG_VEHICLESPATH + " " + classname + " medicalItem") == 1)
		{
			return IsCategoryEnabled(categories, enabledCategories, "medical");
		}

		if (g_Game.IsKindOf(classname, "Edible_Base"))
		{
			return IsCategoryEnabled(categories, enabledCategories, "food");
		}

		if (g_Game.IsKindOf(classname, "Clothing_Base"))
		{
			return IsCategoryEnabled(categories, enabledCategories, "clothing");
		}

		if (g_Game.ConfigGetInt(CFG_VEHICLESPATH + " " + classname + " vehiclePartItem") == 1)
		{
			return IsCategoryEnabled(categories, enabledCategories, "vehicle_parts");
		}

		if (classname.IndexOf("ZenSkills_") == 0)
		{
			return IsCategoryEnabled(categories, enabledCategories, "base_building");
		}

		return IsCategoryEnabled(categories, enabledCategories, "other");
	}

	bool CanSellItem(SilverTrader_Info traderInfo, ItemBase item)
	{
		if (item.IsInherited(FireplaceBase))
			return false;

		if (item.GetHealthLevel() > GameConstants.STATE_WORN)
			return false;

		if (item.IsInherited(Edible_Base))
		{
			Edible_Base edibleBase = Edible_Base.Cast(item);

			if (edibleBase.IsMeat())
				return false;

			if (edibleBase.GetFoodStage())
			{
				int foodStage = edibleBase.GetFoodStage().GetFoodStageType();
				if (foodStage == FoodStageType.BAKED)
					return false;
				if (foodStage == FoodStageType.BOILED)
					return false;
				if (foodStage == FoodStageType.DRIED)
					return false;
				if (foodStage == FoodStageType.BURNED)
					return false;
				if (foodStage == FoodStageType.ROTTEN)
					return false;
			}

			if (edibleBase.GetType().IndexOf("_Opened") != -1)
				return false;

			if (item.GetLiquidTypeInit() == 0 && edibleBase.GetQuantity() != edibleBase.GetQuantityMax())
				return false;
		}

		bool filterResult = false;
		string itemType = item.GetType();

		foreach (string filter : traderInfo.m_sellFilter)
		{
			if (filter.IndexOf("!") == 0)
			{
				string classname = filter.Substring(1, filter.Length() - 1);
				if (itemType == classname || g_Game.ObjectIsKindOf(item, classname))
				{
					filterResult = false;
				}
			}
			else
			{
				if (itemType == filter || g_Game.ObjectIsKindOf(item, filter))
				{
					filterResult = true;
				}
			}
		}

		return filterResult;
	}

	bool CanBuyItem(SilverTrader_Info traderInfo, string itemClassname)
	{
		bool filterResult = false;
		foreach (string filter : traderInfo.m_buyFilter)
		{
			if (filter.IndexOf("!") == 0)
			{
				string classname = filter.Substring(1, filter.Length() - 1);
				if (itemClassname == classname || g_Game.IsKindOf(itemClassname, classname))
				{
					filterResult = false;
				}
			}
			else
			{
				if (itemClassname == filter || g_Game.IsKindOf(itemClassname, filter))
				{
					filterResult = true;
				}
			}
		}

		return filterResult;
	}

	// Debug-Log Hilfsfunktion (nur ausgeben wenn debugMode aktiv)
	void DebugLog(string message)
	{
		if (m_config && m_config.m_debugMode)
		{
			Print("[SilverBarter] " + message);
		}
	}
};
