// Gecachte Config-Daten pro Classname (einmalig befuellt, Config ist statisch)
class SilverItemConfigCache
{
	int    m_ItemCapacity;      // itemSize[0] * itemSize[1], Min 1
	bool   m_IsLiquidContainer;
	int    m_MaxStackSize;      // varQuantityMax oder count
	string m_StackedUnit;       // z.B. "pc."
	bool   m_IsAmmo;            // IsKindOf Ammunition_Base
	bool   m_CanBeSplit;        // m_CanBeSplit == 1 im Config
	string m_Category;        // Ergebnis von FilterByCategories
}

// Laufzeit-Huelle fuer das aktive Loadout eines Rotating-Traders.
// Haelt NUR eine plain-Referenz auf das Config-PoolItem (Eigentuemer bleibt der Pool) plus den eingefrorenen Preis.
class SilverActivePoolEntry
{
	SilverTrader_PoolItem m_PoolItem;   // plain ref -> zeigt auf Config-Objekt, kein Ownership
	int m_UnitSurcharge;                // bei Rotation eingefrorener Stueckaufschlag der Attachments
}

// Arbeitspaket fuer den iterativen Attachment-Spawn (parent + zugehoerige Spec-Liste).
// Iterativ statt rekursiv, weil ein rekursiver Aufruf in diesem Enforce-Build den Schleifenzustand des Aufrufers zerstoert.
class SilverAttachJob
{
	EntityAI m_Parent;                                  // plain ref -> erzeugte Parent-Entity (Spawn)
	array<ref SilverAttachmentSpec> m_Specs;            // plain Handle -> zeigt auf Config-Array (Eigentuemer bleibt die Config)
	int m_ParentIndex;                                  // nur fuer Preview-Listenbau: Index des Parent-Knotens in der flachen Liste
}

// SilverBarter Haupt-Plugin (Server + Client vereint)
class PluginSilverTrader extends PluginBase
{
	// Werkzeug-Klassen fuer Kategorie-Filter (Client + Server)
	static ref array<string> s_ToolClasses;

	// Gueltige Filter-Kategorien (fuer Kategorie-Overrides, verhindert Tippfehler-Ausfaelle)
	static ref array<string> s_ValidCategories;

	// Tiefenversatz fuer die SilverBarterChest-Staging-Position: muss innerhalb der Network-Bubble
	// des Kaeufers bleiben (sonst "[syncinv] item not in bubble"), aber unter der Oberflaeche liegen.
	const float CHEST_STAGING_DEPTH = 2.0;
	const int DELIVERY_POLL_INTERVAL_MS = 200;
	const int DELIVERY_ITEM_MAX_POLLS = 10;   // ~2s Sub-Timeout pro Item, dann Boden
	const int DELIVERY_MAX_POLLS = 60;        // globales Sicherheitsnetz (~12s)

	// Client-Seite
	ref SilverTraderMenu m_SilverBarter_TraderMenu;
	ref array<string> m_SilverBarter_QuantityPriceClassnamesClient;
	ref array<ref SilverCategoryOverride> m_SilverBarter_CategoryOverridesClient;
	ref map<string, float> m_SilverBarter_CategoryValueMultipliersClient;

	// Server-Seite
	SilverBarterConfig m_SilverBarter_Config;
	SilverRotatingTradersConfig m_SilverBarter_RotatingConfig;
	SilverCategoryOverridesConfig m_SilverBarter_CategoryOverridesConfig;
	ref map<int, TraderPoint> m_SilverBarter_TraderPoints;
	ref map<int, SilverTrader_ServerConfig> m_SilverBarter_TraderCache;
	ref map<int, ref SilverTrader_Data> m_SilverBarter_TraderData;
	ref set<int> m_SilverBarter_DirtyTraders;
	float m_SilverBarter_SaveTimer = 0;
	const float SAVE_INTERVAL = 300.0; // Alle 5 Minuten speichern

	// Rotierende Haendler (Runtime-Daten, nicht persistent)
	ref map<int, TraderPoint> m_SilverBarter_RotatingTraderPoints;
	ref map<int, SilverRotatingTrader_Config> m_SilverBarter_RotatingTraderCache;
	ref map<int, ref SilverTrader_Data> m_SilverBarter_RotatingTraderData;
	ref map<int, float> m_SilverBarter_RotationTimers;
	// Aktives Loadout je Trader: traderId -> classname -> Huelle (Baum-Ref + eingefrorener Aufschlag). Server-only.
	ref map<int, ref map<string, ref SilverActivePoolEntry>> m_SilverBarter_ActivePool;
	bool m_SilverBarter_ZenMapMarkersSet = false;

	// Item-Config-Cache (Classname → gecachte Config-Werte)
	ref map<string, ref SilverItemConfigCache> m_SilverBarter_ItemConfigCache;

	// Welche Spieler (PlayerId) haben welchen Trader gerade offen (traderId → PlayerIds)
	ref map<int, ref array<int>> m_SilverBarter_OpenTraderMenus;

	private const string DATA_FOLDER = "$profile:\\SilverBarter\\TraderData\\";

	override void OnInit()
	{
		super.OnInit();

		// Werkzeug-Klassen initialisieren (Client + Server)
		if (!s_ToolClasses)
		{
			s_ToolClasses = new array<string>;
			s_ToolClasses.Insert("Hatchet");
			s_ToolClasses.Insert("Sickle");
			s_ToolClasses.Insert("Blowtorch");
			s_ToolClasses.Insert("Whetstone");
			s_ToolClasses.Insert("ElectronicRepairKit");
			s_ToolClasses.Insert("LugWrench");
			s_ToolClasses.Insert("PipeWrench");
			s_ToolClasses.Insert("Screwdriver");
			s_ToolClasses.Insert("Hacksaw");
			s_ToolClasses.Insert("HandSaw");
			s_ToolClasses.Insert("Pliers");
			s_ToolClasses.Insert("Hammer");
			s_ToolClasses.Insert("CanOpener");
			s_ToolClasses.Insert("SewingKit");
			s_ToolClasses.Insert("LeatherSewingKit");
			s_ToolClasses.Insert("Lockpick");
			s_ToolClasses.Insert("Crowbar");
			s_ToolClasses.Insert("Wrench");
			s_ToolClasses.Insert("SledgeHammer");
			s_ToolClasses.Insert("Cleaver");
			s_ToolClasses.Insert("SteakKnife");
			s_ToolClasses.Insert("KitchenKnife");
			s_ToolClasses.Insert("Broom");
			s_ToolClasses.Insert("Shovel");
			s_ToolClasses.Insert("Pickaxe");
			s_ToolClasses.Insert("Pitchfork");
			s_ToolClasses.Insert("FarmingHoe");
			s_ToolClasses.Insert("OrientalMachete");
			s_ToolClasses.Insert("Machete");
			s_ToolClasses.Insert("FangeKnife");
			s_ToolClasses.Insert("KukriKnife");
			s_ToolClasses.Insert("HuntingKnife");
			s_ToolClasses.Insert("CombatKnife");
			s_ToolClasses.Insert("FirefighterAxe");
			s_ToolClasses.Insert("WoodAxe");
			s_ToolClasses.Insert("Iceaxe");
			s_ToolClasses.Insert("MeatTenderizer");
		}

		if (!s_ValidCategories)
		{
			s_ValidCategories = new array<string>;
			s_ValidCategories.Insert("weapons");
			s_ValidCategories.Insert("magazines");
			s_ValidCategories.Insert("attachments");
			s_ValidCategories.Insert("ammo");
			s_ValidCategories.Insert("tools");
			s_ValidCategories.Insert("food");
			s_ValidCategories.Insert("clothing");
			s_ValidCategories.Insert("medical");
			s_ValidCategories.Insert("electronic");
			s_ValidCategories.Insert("base_building");
			s_ValidCategories.Insert("vehicle_parts");
			s_ValidCategories.Insert("other");
		}

		// Cache einmalig initialisieren (Client + Server)
		m_SilverBarter_ItemConfigCache = new map<string, ref SilverItemConfigCache>;

		// RPC-Handler registrieren (Client + Server)
		SilverRPCManager.RegisterHandler(SilverRPC.SILVERRPC_OPEN_TRADE_MENU, this, "RpcRequestOpen");
		SilverRPCManager.RegisterHandler(SilverRPC.SILVERRPC_ACTION_TRADER, this, "RpcHandleTraderAction");
		SilverRPCManager.RegisterHandler(SilverRPC.SILVERRPC_CLOSE_TRADER_MENU, this, "RpcRequestTraderMenuClose");
		SilverRPCManager.RegisterHandler(SilverRPC.SILVERRPC_ROTATING_TRADER_SYNC, this, "RpcRotatingTraderSync");
		SilverRPCManager.RegisterHandler(SilverRPC.SILVERRPC_DELIVERY_COMPLETE, this, "RpcHandleDeliveryComplete");

		// Server-Initialisierung
		if (g_Game && g_Game.IsDedicatedServer())
		{
			m_SilverBarter_TraderPoints = new map<int, TraderPoint>;
			m_SilverBarter_TraderCache = new map<int, SilverTrader_ServerConfig>;
			m_SilverBarter_TraderData = new map<int, ref SilverTrader_Data>;
			m_SilverBarter_DirtyTraders = new set<int>;

			m_SilverBarter_RotatingTraderPoints = new map<int, TraderPoint>;
			m_SilverBarter_RotatingTraderCache = new map<int, SilverRotatingTrader_Config>;
			m_SilverBarter_RotatingTraderData = new map<int, ref SilverTrader_Data>;
			m_SilverBarter_RotationTimers = new map<int, float>;
			m_SilverBarter_ActivePool = new map<int, ref map<string, ref SilverActivePoolEntry>>;
			m_SilverBarter_OpenTraderMenus = new map<int, ref array<int>>;

			m_SilverBarter_Config = SilverBarterConfigService.GetConfig();
			m_SilverBarter_RotatingConfig = SilverBarterConfigService.GetRotatingConfig();
			m_SilverBarter_CategoryOverridesConfig = SilverBarterConfigService.GetCategoryOverridesConfig();
		}
	}

	// ========== CLIENT-SEITE ==========

	void CloseTraderMenu()
	{
		if (m_SilverBarter_TraderMenu && m_SilverBarter_TraderMenu.m_SilverBarter_Active)
		{
			m_SilverBarter_TraderMenu.m_SilverBarter_Active = false;
		}
	}

	// Wird von SilverTraderMenu.OnHide() aufgerufen, um die Plugin-Referenz freizugeben
	void ClearTraderMenuRef(SilverTraderMenu menu)
	{
		if (m_SilverBarter_TraderMenu == menu)
			m_SilverBarter_TraderMenu = null;
	}

	// Client: RPC empfangen - Menu oeffnen
	void RpcRequestOpen(ParamsReadContext ctx, PlayerIdentity sender)
	{
		if (!g_Game || g_Game.IsDedicatedServer())
			return;

		if (m_SilverBarter_TraderMenu && m_SilverBarter_TraderMenu.m_SilverBarter_Active)
		{
			m_SilverBarter_TraderMenu.m_SilverBarter_Active = false;
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
		traderInfo.m_commissionOverrides = new map<string, float>;
		for (int ov = 0; ov < overrideCount; ov++)
		{
			string ovClass;
			float ovComm;
			if (!ctx.Read(ovClass)) return;
			if (!ctx.Read(ovComm)) return;
			traderInfo.m_commissionOverrides.Set(ovClass, ovComm);
		}

		// Trader-spezifische Kategorie-Wert-Multiplikatoren lesen (optional)
		int traderCatValueMultiplierCount;
		if (!ctx.Read(traderCatValueMultiplierCount)) return;
		traderInfo.m_categoryValueMultipliers = new map<string, float>;
		for (int tcm = 0; tcm < traderCatValueMultiplierCount; tcm++)
		{
			string tcmCategory;
			float tcmMultiplier;
			if (!ctx.Read(tcmCategory)) return;
			if (!ctx.Read(tcmMultiplier)) return;

			if (tcmCategory == "" || tcmMultiplier <= 0 || !IsValidCategory(tcmCategory))
				continue;

			tcmCategory.ToLower();
			traderInfo.m_categoryValueMultipliers.Set(tcmCategory, tcmMultiplier);
		}

		// QuantityPrice-Classnames lesen (global, client-only)
		int qpCount;
		if (!ctx.Read(qpCount)) return;
		m_SilverBarter_QuantityPriceClassnamesClient = new array<string>;
		for (int qp = 0; qp < qpCount; qp++)
		{
			string qpClass;
			if (!ctx.Read(qpClass)) return;
			m_SilverBarter_QuantityPriceClassnamesClient.Insert(qpClass);
		}

		// Kategorie-Overrides lesen (global, client-only)
		int catOverrideCount;
		if (!ctx.Read(catOverrideCount)) return;
		m_SilverBarter_CategoryOverridesClient = new array<ref SilverCategoryOverride>;
		for (int co = 0; co < catOverrideCount; co++)
		{
			string ovPattern, ovCategory;
			bool ovPrefixOnly;
			if (!ctx.Read(ovPattern)) return;
			if (!ctx.Read(ovCategory)) return;
			if (!ctx.Read(ovPrefixOnly)) return;

			SilverCategoryOverride catOverrideEntry = new SilverCategoryOverride();
			catOverrideEntry.pattern = ovPattern;
			catOverrideEntry.category = ovCategory;
			catOverrideEntry.prefixOnly = ovPrefixOnly;
			m_SilverBarter_CategoryOverridesClient.Insert(catOverrideEntry);
		}

		// Kategorie-Wert-Multiplikatoren lesen (global, client-only)
		int catValueMultiplierCount;
		if (!ctx.Read(catValueMultiplierCount)) return;
		m_SilverBarter_CategoryValueMultipliersClient = new map<string, float>;
		for (int cm = 0; cm < catValueMultiplierCount; cm++)
		{
			string cmCategory;
			float cmMultiplier;
			if (!ctx.Read(cmCategory)) return;
			if (!ctx.Read(cmMultiplier)) return;

			if (cmCategory == "" || cmMultiplier <= 0 || !IsValidCategory(cmCategory))
				continue;

			cmCategory.ToLower();
			m_SilverBarter_CategoryValueMultipliersClient.Set(cmCategory, cmMultiplier);
		}

		// Bereits gecachte Kategorien koennten ohne Override-Kenntnis berechnet worden sein
		if (m_SilverBarter_ItemConfigCache)
			m_SilverBarter_ItemConfigCache.Clear();

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

		// Rotation-Revision + Attachment-Aufschlaege
		if (!ReadAttachmentSync(ctx, traderData)) return;

		m_SilverBarter_TraderMenu = new SilverTraderMenu;
		m_SilverBarter_TraderMenu.InitMetadata(traderInfo.m_traderId, traderInfo, traderData, isRotating);
		g_Game.GetUIManager().ShowScriptedMenu(m_SilverBarter_TraderMenu, null);
	}

	// Client: RPC empfangen - Trade-Antwort
	void RpcHandleTraderAction(ParamsReadContext ctx, PlayerIdentity sender)
	{
		// Server-Handler
		if (g_Game && g_Game.IsDedicatedServer())
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

		// Rotation-Revision + Attachment-Aufschlaege
		if (!ReadAttachmentSync(ctx, newData)) return;

		if (m_SilverBarter_TraderMenu && m_SilverBarter_TraderMenu.m_SilverBarter_Active)
		{
			m_SilverBarter_TraderMenu.UpdateMetadata(newData);
			m_SilverBarter_TraderMenu.ClearBuySelection();
			// Sell-Refresh haengt NICHT mehr an dieser fruehen Trade-Antwort, sondern am
			// spaeteren SILVERRPC_DELIVERY_COMPLETE - erst dann sind die Kauf-Items zugestellt.
		}
	}

	// Client: RPC empfangen - serielle Zustellung serverseitig abgeschlossen.
	// Kein sofortiger Rebuild: RPC und Inventar-Replikation laufen ueber getrennte Kanaele, deren
	// clientseitige Verarbeitungsreihenfolge nicht garantiert ist. Deshalb nur "dirty" markieren und
	// mit kleinem Sync-Puffer refreshen (siehe ScheduleSellRefresh).
	void RpcHandleDeliveryComplete(ParamsReadContext ctx, PlayerIdentity sender)
	{
		if (!g_Game || g_Game.IsDedicatedServer())
			return;

		if (m_SilverBarter_TraderMenu && m_SilverBarter_TraderMenu.m_SilverBarter_Active)
			m_SilverBarter_TraderMenu.ScheduleSellRefresh();
	}

	// ========== SERVER-SEITE ==========

	void InitializeTraders()
	{
		if (!g_Game || !g_Game.IsDedicatedServer())
			return;

		if (!m_SilverBarter_Config || !m_SilverBarter_Config.m_traders)
		{
			Print("[SilverBarter] ERROR: Config not loaded!");
			return;
		}

		// Data-Ordner erstellen
		if (!FileExist(DATA_FOLDER))
		{
			MakeDirectory(DATA_FOLDER);
		}

		set<int> seenIds = new set<int>;

		foreach (SilverTrader_ServerConfig trader : m_SilverBarter_Config.m_traders)
		{
			if (!trader || !trader.ValidateAndNormalize())
			{
				Print("[SilverBarter] ERROR: Trader config invalid, skipped.");
				continue;
			}
			if (seenIds.Find(trader.m_traderId) != -1)
			{
				Print("[SilverBarter] ERROR: Duplicate trader ID skipped: " + trader.m_traderId.ToString());
				continue;
			}
			seenIds.Insert(trader.m_traderId);
			SpawnTrader(trader);
		}

		DebugLog(m_SilverBarter_Config.m_traders.Count().ToString() + " Trader initialisiert.");

		// Rotierende Haendler initialisieren (gleiche seenIds - Trader-IDs sind global eindeutig)
		if (m_SilverBarter_RotatingConfig && m_SilverBarter_RotatingConfig.m_rotatingTraders && m_SilverBarter_RotatingConfig.m_rotatingTraders.Count() > 0)
		{
			foreach (SilverRotatingTrader_Config rotTrader : m_SilverBarter_RotatingConfig.m_rotatingTraders)
			{
				if (!rotTrader || !rotTrader.ValidateAndNormalize())
				{
					Print("[SilverBarter] ERROR: Rotating trader config invalid, skipped.");
					continue;
				}
				if (seenIds.Find(rotTrader.m_traderId) != -1)
				{
					Print("[SilverBarter] ERROR: Duplicate trader ID skipped: " + rotTrader.m_traderId.ToString());
					continue;
				}
				seenIds.Insert(rotTrader.m_traderId);
				SpawnRotatingTrader(rotTrader);
			}
			DebugLog(m_SilverBarter_RotatingConfig.m_rotatingTraders.Count().ToString() + " Rotating Trader initialisiert.");
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
			if (!traderData.LoadFromJson(dataPath))
			{
				Print("[SilverBarter] ERROR: Trader data load failed, trader skipped to prevent stock overwrite: " + dataPath);
				return;
			}
		}
		else if (trader.m_defaultItems && trader.m_defaultItems.Count() > 0)
		{
			// Default-Items laden
			traderData.m_items = new map<string, float>;
			foreach (string defaultClassname, float defaultQuantity : trader.m_defaultItems)
			{
				if (defaultClassname != "")
					traderData.m_items.Insert(defaultClassname, defaultQuantity);
			}
			traderData.SaveToJson(dataPath);
			DebugLog("Default items for trader " + trader.m_traderId.ToString() + " loaded.");
		}

		// Limitierte Items bei jedem Restart auf maxQuantity zuruecksetzen
		if (trader.m_limitedItems && trader.m_limitedItems.Count() > 0)
		{
			bool limitedChanged = false;
			foreach (string limitedClassname, int limitedMaxQuantity : trader.m_limitedItems)
			{
				if (limitedClassname != "")
				{
					if (traderData.m_items.Contains(limitedClassname))
					{
						float currentLimitedQuantity = traderData.m_items.Get(limitedClassname);
						if (currentLimitedQuantity != limitedMaxQuantity)
						{
							traderData.m_items.Set(limitedClassname, limitedMaxQuantity);
							limitedChanged = true;
						}
					}
					else
					{
						traderData.m_items.Insert(limitedClassname, limitedMaxQuantity);
						limitedChanged = true;
					}
				}
			}
			if (limitedChanged)
			{
				traderData.SaveToJson(dataPath);
				DebugLog("Limited items for trader " + trader.m_traderId.ToString() + " reset.");
			}
		}

		m_SilverBarter_TraderData.Insert(trader.m_traderId, traderData);

		// Trader-NPC spawnen
		Object traderObj = g_Game.CreateObject(trader.m_classname, trader.m_position);
		if (!traderObj)
		{
			Print("[SilverBarter] ERROR: Could not spawn trader NPC: " + trader.m_classname + " – cleaning up traderData.");
			m_SilverBarter_TraderData.Remove(trader.m_traderId);
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

		if (!traderPoint)
		{
			Print("[SilverBarter] ERROR: TraderPoint creation failed for trader " + trader.m_traderId.ToString() + " – cleaning up NPC and traderData.");
			g_Game.ObjectDelete(traderObj);
			m_SilverBarter_TraderData.Remove(trader.m_traderId);
			return;
		}

		m_SilverBarter_TraderPoints.Insert(trader.m_traderId, traderPoint);
		m_SilverBarter_TraderCache.Insert(trader.m_traderId, trader);

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

		if (m_SilverBarter_RotatingTraderCache.Contains(trader.m_traderId))
		{
			Print("[SilverBarter] ERROR: Duplicate rotating trader ID, skipped: " + trader.m_traderId.ToString());
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

		// Runtime-Daten lokal erstellen (noch nicht in Maps eintragen)
		SilverTrader_Data traderData = new SilverTrader_Data();
		RotateTraderPool(trader, traderData);

		// NPC spawnen
		Object traderObj = g_Game.CreateObject(trader.m_classname, spawnPos);
		if (!traderObj)
		{
			Print("[SilverBarter] ERROR: Could not spawn rotating trader NPC: " + trader.m_classname);
			if (m_SilverBarter_ActivePool)
				m_SilverBarter_ActivePool.Remove(trader.m_traderId);
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
		if (!traderPoint)
		{
			Print("[SilverBarter] ERROR: TraderPoint creation failed for rotating trader " + trader.m_traderId.ToString() + " - cleaning up NPC.");
			g_Game.ObjectDelete(traderObj);
			if (m_SilverBarter_ActivePool)
				m_SilverBarter_ActivePool.Remove(trader.m_traderId);
			return;
		}

		traderPoint.SetAllowDamage(false);
		traderPoint.SetPosition(spawnPos);
		traderPoint.SetOrientation(Vector(trader.m_orientation, 0, 0));
		traderPoint.InitTraderPoint(trader.m_traderId, traderObj, true);

		// Erst nach erfolgreichem NPC- und TraderPoint-Spawn in alle Maps eintragen
		m_SilverBarter_RotatingTraderData.Insert(trader.m_traderId, traderData);
		m_SilverBarter_RotatingTraderPoints.Insert(trader.m_traderId, traderPoint);
		m_SilverBarter_RotatingTraderCache.Insert(trader.m_traderId, trader);
		m_SilverBarter_RotationTimers.Insert(trader.m_traderId, 0);

		DebugLog("Rotating Trader " + trader.m_traderId.ToString() + " spawned with " + traderData.m_items.Count().ToString() + " items.");
	}

	private void RotateTraderPool(SilverRotatingTrader_Config trader, SilverTrader_Data traderData)
	{
		if (!trader || !traderData)
			return;

		int traderId = trader.m_traderId;

		// Aktives Loadout fuer diesen Trader neu aufbauen (traderId -> classname -> Huelle)
		map<string, ref SilverActivePoolEntry> activeEntries = new map<string, ref SilverActivePoolEntry>;
		if (m_SilverBarter_ActivePool)
			m_SilverBarter_ActivePool.Set(traderId, activeEntries);

		if (!traderData.m_items)
			traderData.m_items = new map<string, float>;
		else
			traderData.m_items.Clear();

		if (!traderData.m_attachmentSurcharge)
			traderData.m_attachmentSurcharge = new map<string, int>;
		else
			traderData.m_attachmentSurcharge.Clear();

		if (!traderData.m_previewAttachments)
			traderData.m_previewAttachments = new map<string, ref array<ref SilverPreviewAttachment>>;
		else
			traderData.m_previewAttachments.Clear();

		// Revision hochzaehlen: Client-Loadout und Server-Loadout muessen beim Kauf uebereinstimmen
		traderData.m_rotationRevision = traderData.m_rotationRevision + 1;

		if (!trader.m_poolItems || trader.m_poolItems.Count() == 0)
			return;

		int slotsToFill = Math.Min(trader.m_activeSlots, trader.m_poolItems.Count());

		// Kopie der Pool-Indizes erstellen fuer gewichtete Auswahl ohne Zuruecklegen
		array<int> availableIndices = new array<int>;
		array<float> availableWeights = new array<float>;
		float totalWeight = 0;

		for (int i = 0; i < trader.m_poolItems.Count(); i++)
		{
			SilverTrader_PoolItem poolItem = trader.m_poolItems.Get(i);
			if (poolItem && poolItem.weight > 0 && poolItem.classname != "")
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

			// Gewaehlten Slot aus den Kandidaten nehmen
			totalWeight = totalWeight - availableWeights.Get(selectedIdx);
			availableIndices.Remove(selectedIdx);
			availableWeights.Remove(selectedIdx);

			if (!selected)
				continue;

			// Alle uebrigen Kandidaten mit demselben classname entfernen: die Stock-Map kann pro classname
			// nur einen Eintrag halten, also darf pro Rotation nur eine Variante aktiv sein.
			for (int d = availableIndices.Count() - 1; d >= 0; d--)
			{
				int candIndex = availableIndices.Get(d);
				SilverTrader_PoolItem candidate = trader.m_poolItems.Get(candIndex);
				if (candidate && candidate.classname == selected.classname)
				{
					totalWeight = totalWeight - availableWeights.Get(d);
					availableIndices.Remove(d);
					availableWeights.Remove(d);
				}
			}

			traderData.m_items.Insert(selected.classname, selected.quantity);

			// Attachment-Aufschlag einmalig einfrieren (stock=0, pro Knoten gefloort)
			int surcharge = CalculateAttachmentTreeSurcharge(trader, selected.attachments);

			SilverActivePoolEntry entry = new SilverActivePoolEntry();
			entry.m_PoolItem = selected;
			entry.m_UnitSurcharge = surcharge;
			activeEntries.Insert(selected.classname, entry);

			if (surcharge > 0)
				traderData.m_attachmentSurcharge.Insert(selected.classname, surcharge);

			// Flache Preview-Beschreibung fuer den Client (nur wenn Attachments vorhanden)
			if (selected.attachments && selected.attachments.Count() > 0)
				traderData.m_previewAttachments.Insert(selected.classname, BuildPreviewList(selected.attachments));

			DebugLog("Rotation: Selected " + selected.classname + " (qty: " + selected.quantity.ToString() + ", surcharge: " + surcharge.ToString() + ")");
		}
	}

	// Aktiven Pool-Eintrag (Baum + eingefrorener Preis) fuer traderId + classname holen. Server-only. null wenn nicht vorhanden.
	private SilverActivePoolEntry GetActivePoolEntry(int traderId, string classname)
	{
		if (!m_SilverBarter_ActivePool)
			return null;

		map<string, ref SilverActivePoolEntry> entries;
		if (!m_SilverBarter_ActivePool.Find(traderId, entries) || !entries)
			return null;

		SilverActivePoolEntry entry;
		if (entries.Find(classname, entry))
			return entry;
		return null;
	}

	// Haengt einen Attachment-Baum an parent an. Liefert false bei erstem Fehlschlag (-> Trade-Rollback).
	// Bewusst iterativ ueber eine Work-Queue statt rekursiv: ein rekursiver Aufruf im Schleifenkoerper zerstoert
	// in diesem Enforce-Build den Schleifenzustand des Aufrufers (Loop bricht nach der ersten Iteration ab).
	private bool SpawnAttachmentTree(EntityAI rootParent, array<ref SilverAttachmentSpec> rootSpecs)
	{
		if (!rootParent || !rootSpecs)
			return true;

		array<ref SilverAttachJob> queue = new array<ref SilverAttachJob>;
		SilverAttachJob rootJob = new SilverAttachJob();
		rootJob.m_Parent = rootParent;
		rootJob.m_Specs = rootSpecs;
		queue.Insert(rootJob);

		while (queue.Count() > 0)
		{
			SilverAttachJob job = queue.Get(0);
			queue.Remove(0);
			if (!job || !job.m_Parent || !job.m_Specs)
				continue;

			EntityAI parent = job.m_Parent;
			array<ref SilverAttachmentSpec> specs = job.m_Specs;

			int count = specs.Count();
			DebugLog("SpawnAttachmentTree: parent=" + parent.GetType() + " count=" + count.ToString());

			for (int i = 0; i < count; i++)
			{
				SilverAttachmentSpec spec = specs.Get(i);
				if (!spec || spec.classname == "")
					continue;

				EntityAI attachment = SpawnSingleAttachment(parent, spec);

				if (!attachment)
				{
					Print("[SilverBarter] SPAWN FAILED: attachment " + spec.classname + " could not be attached to " + parent.GetType());
					return false;
				}

				ApplyAttachmentFill(attachment, spec.fill);

				// Verschachtelte Attachments (z.B. Batterie in Optik) als eigenes Arbeitspaket einreihen,
				// statt hier rekursiv zu spawnen.
				if (spec.attachments && spec.attachments.Count() > 0)
				{
					SilverAttachJob childJob = new SilverAttachJob();
					childJob.m_Parent = attachment;
					childJob.m_Specs = spec.attachments;
					queue.Insert(childJob);
				}
			}
		}
		return true;
	}

	// Erzeugt ein einzelnes Attachment an parent. Nutzt LocationCreateEntity (erzeugt direkt an der Ziel-Location,
	// tick-sicher). Ohne festen Slot wird die passende Attachment-Location ueber eine temporaere Entity ermittelt -
	// FindFirstFreeLocationForNewEntity(classname) ist fuer Attachments an einem Item-im-Cargo unzuverlaessig.
	private EntityAI SpawnSingleAttachment(EntityAI parent, SilverAttachmentSpec spec)
	{
		// Magazine haben ein eigenes Waffen-System (magazines[]-Config), das FindFreeLocationFor(ATTACHMENT) nicht
		// abdeckt. CreateAttachment findet den Magazin-Slot dagegen korrekt. Ammunition_Base liegt ebenfalls unter
		// CfgMagazines, ist aber kein Waffenmagazin und muss ausgeschlossen werden. Ein evtl. gesetzter slot wird
		// hier bewusst ignoriert - der Magazin-Slot ergibt sich aus der Waffen-Config.
		bool isMagazine = g_Game.IsKindOf(spec.classname, "Magazine_Base");
		bool isAmmunition = g_Game.IsKindOf(spec.classname, "Ammunition_Base");

		Weapon_Base weapon = Weapon_Base.Cast(parent);
		if (weapon && isMagazine && !isAmmunition)
		{
			EntityAI attachedMagazine = parent.GetInventory().CreateAttachment(spec.classname);
			DebugLog("SpawnSingle(magazine): " + spec.classname + " created=" + (attachedMagazine != null).ToString());
			return attachedMagazine;
		}

		InventoryLocation loc = new InventoryLocation;

		// Fester Slot aus der Config
		if (spec.slot != "")
		{
			int slotId = InventorySlots.GetSlotIdFromString(spec.slot);
			if (slotId == InventorySlots.INVALID)
				return null;
			loc.SetAttachment(parent, null, slotId);
			EntityAI slotAtt = GameInventory.LocationCreateEntity(loc, spec.classname, ECE_IN_INVENTORY, RF_DEFAULT);
			DebugLog("SpawnSingle(slot): " + spec.classname + " slotId=" + slotId.ToString() + " created=" + (slotAtt != null).ToString());
			return slotAtt;
		}

		// Kein Slot: passende Attachment-Location ueber eine temporaere Entity ermitteln, dann verwerfen und
		// die echte Entity direkt an der Location erzeugen (bewaehrter Expansion-Ansatz).
		Object tmpObj = g_Game.CreateObjectEx(spec.classname, "0 0 0", ECE_LOCAL);
		EntityAI tmpEntity;
		if (!Class.CastTo(tmpEntity, tmpObj))
		{
			if (tmpObj)
				g_Game.ObjectDelete(tmpObj);
			DebugLog("SpawnSingle: " + spec.classname + " tmp create failed");
			return null;
		}

		bool found = parent.GetInventory().FindFreeLocationFor(tmpEntity, FindInventoryLocationType.ATTACHMENT, loc);
		int foundSlot = -1;
		EntityAI created = null;
		if (found)
		{
			foundSlot = loc.GetSlot();
			created = GameInventory.LocationCreateEntity(loc, spec.classname, ECE_IN_INVENTORY, RF_DEFAULT);
		}
		DebugLog("SpawnSingle: " + spec.classname + " found=" + found.ToString() + " slot=" + foundSlot.ToString() + " created=" + (created != null).ToString());

		g_Game.ObjectDelete(tmpObj);
		return created;
	}

	// Wendet den Fuellgrad auf ein frisch erzeugtes Attachment an. Negatives fill = Config-Default belassen.
	private void ApplyAttachmentFill(EntityAI attachment, float fill)
	{
		if (fill < 0)
			return;

		float clamped = Math.Clamp(fill, 0, 1);

		Magazine mag;
		if (Class.CastTo(mag, attachment))
		{
			mag.ServerSetAmmoCount((int)Math.Round(mag.GetAmmoMax() * clamped));
			return;
		}

		ItemBase item;
		if (ItemBase.CastTo(item, attachment))
		{
			item.SetQuantityNormalized(clamped);
		}
	}

	// Schreibt Rotation-Revision + Attachment-Aufschlaege (classname->int) in den RPC.
	// Reihenfolge-kritisch: muss exakt zu ReadAttachmentSync passen.
	private void WriteAttachmentSync(ScriptRPC rpc, SilverTrader_Data data)
	{
		int revision = 0;
		int surchargeCount = 0;
		if (data)
		{
			revision = data.m_rotationRevision;
			if (data.m_attachmentSurcharge)
				surchargeCount = data.m_attachmentSurcharge.Count();
		}

		rpc.Write(revision);
		rpc.Write(surchargeCount);
		if (surchargeCount > 0)
		{
			for (int i = 0; i < data.m_attachmentSurcharge.Count(); i++)
			{
				rpc.Write(data.m_attachmentSurcharge.GetKey(i));
				rpc.Write(data.m_attachmentSurcharge.GetElement(i));
			}
		}

		// Preview-Baum je Waffen-classname (flach: classname, slot, parentIndex)
		int previewCount = 0;
		if (data && data.m_previewAttachments)
			previewCount = data.m_previewAttachments.Count();
		rpc.Write(previewCount);
		for (int p = 0; p < previewCount; p++)
		{
			string weaponClass = data.m_previewAttachments.GetKey(p);
			array<ref SilverPreviewAttachment> list = data.m_previewAttachments.GetElement(p);
			rpc.Write(weaponClass);

			int attCount = 0;
			if (list)
				attCount = list.Count();
			rpc.Write(attCount);

			for (int a = 0; a < attCount; a++)
			{
				SilverPreviewAttachment pa = list.Get(a);
				rpc.Write(pa.m_Classname);
				rpc.Write(pa.m_Slot);
				rpc.Write(pa.m_ParentIndex);
			}
		}
	}

	// Liest Rotation-Revision + Attachment-Aufschlaege und legt sie in data ab. false bei Lesefehler.
	private bool ReadAttachmentSync(ParamsReadContext ctx, SilverTrader_Data data)
	{
		int revision;
		if (!ctx.Read(revision)) return false;

		int surchargeCount;
		if (!ctx.Read(surchargeCount)) return false;

		if (data)
		{
			data.m_rotationRevision = revision;
			if (!data.m_attachmentSurcharge)
				data.m_attachmentSurcharge = new map<string, int>;
			else
				data.m_attachmentSurcharge.Clear();
		}

		for (int i = 0; i < surchargeCount; i++)
		{
			string surchargeClass;
			int surchargeValue;
			if (!ctx.Read(surchargeClass)) return false;
			if (!ctx.Read(surchargeValue)) return false;
			if (data && data.m_attachmentSurcharge)
				data.m_attachmentSurcharge.Set(surchargeClass, surchargeValue);
		}

		if (data)
		{
			if (!data.m_previewAttachments)
				data.m_previewAttachments = new map<string, ref array<ref SilverPreviewAttachment>>;
			else
				data.m_previewAttachments.Clear();
		}

		int previewCount;
		if (!ctx.Read(previewCount)) return false;
		for (int p = 0; p < previewCount; p++)
		{
			string weaponClass;
			if (!ctx.Read(weaponClass)) return false;

			int attCount;
			if (!ctx.Read(attCount)) return false;

			array<ref SilverPreviewAttachment> list = new array<ref SilverPreviewAttachment>;
			for (int a = 0; a < attCount; a++)
			{
				SilverPreviewAttachment pa = new SilverPreviewAttachment();
				if (!ctx.Read(pa.m_Classname)) return false;
				if (!ctx.Read(pa.m_Slot)) return false;
				if (!ctx.Read(pa.m_ParentIndex)) return false;
				list.Insert(pa);
			}

			if (data && data.m_previewAttachments)
				data.m_previewAttachments.Set(weaponClass, list);
		}
		return true;
	}

	private void CheckRotationTimers(float delta_time)
	{
		if (!m_SilverBarter_RotatingTraderCache || !m_SilverBarter_RotationTimers)
			return;

		foreach (int traderId, SilverRotatingTrader_Config config : m_SilverBarter_RotatingTraderCache)
		{
			if (!config || config.m_rotationIntervalMinutes <= 0)
				continue;

			float timer = 0;
			if (m_SilverBarter_RotationTimers.Contains(traderId))
			{
				timer = m_SilverBarter_RotationTimers.Get(traderId);
			}

			timer = timer + delta_time;
			float intervalSeconds = config.m_rotationIntervalMinutes * 60.0;

			if (timer >= intervalSeconds)
			{
				timer = 0;

				SilverTrader_Data traderData;
				if (m_SilverBarter_RotatingTraderData.Find(traderId, traderData))
				{
					RotateTraderPool(config, traderData);
					SyncRotatingTraderToClients(traderId);
					DebugLog("Rotating Trader " + traderId.ToString() + " pool rotated.");
				}
			}

			m_SilverBarter_RotationTimers.Set(traderId, timer);
		}
	}

	private void SyncRotatingTraderToClients(int traderId)
	{
		SilverTrader_Data traderData;
		if (!m_SilverBarter_RotatingTraderData.Find(traderId, traderData))
			return;

		// Nur Spieler mit offenem Menue dieses Traders benachrichtigen
		array<int> viewerIds;
		if (!m_SilverBarter_OpenTraderMenus || !m_SilverBarter_OpenTraderMenus.Find(traderId, viewerIds) || !viewerIds || viewerIds.Count() == 0)
			return;

		// Items einmal zusammenstellen (fuer alle Viewer identisch)
		int itemCount = 0;
		if (traderData && traderData.m_items)
			itemCount = traderData.m_items.Count();

		array<Man> players = new array<Man>;
		g_Game.GetPlayers(players);

		// Online-Spieler als Set aufbauen (fuer Garbage-Collect)
		array<int> onlineIds = new array<int>;
		foreach (Man onlineMan : players)
		{
			PlayerBase onlinePlayer = PlayerBase.Cast(onlineMan);
			if (onlinePlayer && onlinePlayer.GetIdentity())
				onlineIds.Insert(onlinePlayer.GetIdentity().GetPlayerId());
		}

		// Nicht mehr verbundene Viewer bereinigen
		for (int vi = viewerIds.Count() - 1; vi >= 0; vi--)
		{
			if (onlineIds.Find(viewerIds.Get(vi)) == -1)
				viewerIds.Remove(vi);
		}

		if (viewerIds.Count() == 0)
		{
			m_SilverBarter_OpenTraderMenus.Remove(traderId);
			return;
		}

		foreach (Man man : players)
		{
			PlayerBase player = PlayerBase.Cast(man);
			if (!player || !player.GetIdentity())
				continue;

			if (viewerIds.Find(player.GetIdentity().GetPlayerId()) == -1)
				continue;

			ScriptRPC rpc = new ScriptRPC();
			rpc.Write(SilverRPC.SILVERRPC_ROTATING_TRADER_SYNC);
			rpc.Write(traderId);
			rpc.Write(itemCount);
			if (traderData && traderData.m_items)
			{
				for (int k = 0; k < traderData.m_items.Count(); k++)
				{
					rpc.Write(traderData.m_items.GetKey(k));
					rpc.Write(traderData.m_items.GetElement(k));
				}
			}

			// Rotation-Revision + Attachment-Aufschlaege
			WriteAttachmentSync(rpc, traderData);

			rpc.Send(player, SilverRPCManager.CHANNEL_SILVER_BARTER, true, player.GetIdentity());
		}
	}

	// Client: Empfaengt neues Rotating-Trader Inventar nach Rotation
	void RpcRotatingTraderSync(ParamsReadContext ctx, PlayerIdentity sender)
	{
		if (!g_Game || g_Game.IsDedicatedServer())
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

		// Rotation-Revision + Attachment-Aufschlaege
		if (!ReadAttachmentSync(ctx, newData)) return;

		if (m_SilverBarter_TraderMenu && m_SilverBarter_TraderMenu.m_SilverBarter_Active && m_SilverBarter_TraderMenu.m_SilverBarter_TraderId == traderId)
		{
			m_SilverBarter_TraderMenu.UpdateMetadata(newData);
		}
	}

	// Prueft ob eine traderId zu einem rotierenden Haendler gehoert
	bool IsRotatingTrader(int traderId)
	{
		if (m_SilverBarter_RotatingTraderCache && m_SilverBarter_RotatingTraderCache.Contains(traderId))
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

		if (!g_Game || !g_Game.IsDedicatedServer())
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
			if (!m_SilverBarter_RotatingTraderCache.Find(traderId, rotConfig))
				return;
			trader = rotConfig;
			if (!m_SilverBarter_RotatingTraderData.Find(traderId, traderData))
				return;
		}
		else
		{
			SilverTrader_ServerConfig stdConfig;
			if (!m_SilverBarter_TraderCache.Find(traderId, stdConfig))
				return;
			trader = stdConfig;
			if (!m_SilverBarter_TraderData.Find(traderId, traderData))
				return;
		}

		TraderPoint traderPoint;
		if (isRotating)
		{
			if (!m_SilverBarter_RotatingTraderPoints.Find(traderId, traderPoint))
				return;
		}
		else
		{
			if (!m_SilverBarter_TraderPoints.Find(traderId, traderPoint))
				return;
		}

		// Spieler als aktiver Viewer dieses Traders eintragen (fuer selektiven Sync)
		int openPlayerId = player.GetIdentity().GetPlayerId();
		foreach (int oldTid, array<int> oldViewers : m_SilverBarter_OpenTraderMenus)
		{
			oldViewers.RemoveItem(openPlayerId);
		}
		array<int> viewers;
		if (!m_SilverBarter_OpenTraderMenus.Find(traderId, viewers))
		{
			viewers = new array<int>;
			m_SilverBarter_OpenTraderMenus.Insert(traderId, viewers);
		}
		if (viewers.Find(openPlayerId) == -1)
			viewers.Insert(openPlayerId);

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
		if (overrideCount > 0)
		{
			foreach (string overrideClassname, float overrideCommission : trader.m_commissionOverrides)
			{
				rpc.Write(overrideClassname);
				rpc.Write(overrideCommission);
			}
		}

		// Trader-spezifische Kategorie-Wert-Multiplikatoren (optional; ungueltige Eintraege ueberspringen)
		int traderCatValueMultiplierCount = 0;
		if (trader.m_categoryValueMultipliers)
		{
			foreach (string countTraderCategory, float countTraderMultiplier : trader.m_categoryValueMultipliers)
			{
				if (countTraderCategory != "" && countTraderMultiplier > 0 && IsValidCategory(countTraderCategory))
					traderCatValueMultiplierCount++;
			}
		}
		rpc.Write(traderCatValueMultiplierCount);

		if (traderCatValueMultiplierCount > 0)
		{
			foreach (string writeTraderCategory, float writeTraderMultiplier : trader.m_categoryValueMultipliers)
			{
				if (writeTraderCategory == "" || writeTraderMultiplier <= 0 || !IsValidCategory(writeTraderCategory))
					continue;

				rpc.Write(writeTraderCategory);
				rpc.Write(writeTraderMultiplier);
			}
		}

		// QuantityPrice-Classnames (global)
		int qpCount = 0;
		if (m_SilverBarter_Config && m_SilverBarter_Config.m_quantityPriceClassnames)
			qpCount = m_SilverBarter_Config.m_quantityPriceClassnames.Count();
		rpc.Write(qpCount);
		for (int qp = 0; qp < qpCount; qp++)
		{
			rpc.Write(m_SilverBarter_Config.m_quantityPriceClassnames.Get(qp));
		}

		// Kategorie-Overrides (global, nur wenn aktiviert; ungueltige Eintraege ueberspringen)
		int catOverrideCount = 0;
		if (m_SilverBarter_CategoryOverridesConfig && m_SilverBarter_CategoryOverridesConfig.m_enabled && m_SilverBarter_CategoryOverridesConfig.m_categoryOverrides)
		{
			foreach (SilverCategoryOverride countEntry : m_SilverBarter_CategoryOverridesConfig.m_categoryOverrides)
			{
				if (countEntry && countEntry.pattern != "" && countEntry.category != "")
					catOverrideCount++;
			}
		}
		rpc.Write(catOverrideCount);

		if (catOverrideCount > 0)
		{
			foreach (SilverCategoryOverride writeEntry : m_SilverBarter_CategoryOverridesConfig.m_categoryOverrides)
			{
				if (!writeEntry || writeEntry.pattern == "" || writeEntry.category == "")
					continue;

				rpc.Write(writeEntry.pattern);
				rpc.Write(writeEntry.category);
				rpc.Write(writeEntry.prefixOnly);
			}
		}

		// Kategorie-Wert-Multiplikatoren (global; ungueltige Eintraege ueberspringen)
		int catValueMultiplierCount = 0;
		if (m_SilverBarter_Config && m_SilverBarter_Config.m_categoryValueMultipliers)
		{
			foreach (string countCategory, float countMultiplier : m_SilverBarter_Config.m_categoryValueMultipliers)
			{
				if (countCategory != "" && countMultiplier > 0 && IsValidCategory(countCategory))
					catValueMultiplierCount++;
			}
		}
		rpc.Write(catValueMultiplierCount);

		if (catValueMultiplierCount > 0)
		{
			foreach (string writeCategory, float writeMultiplier : m_SilverBarter_Config.m_categoryValueMultipliers)
			{
				if (writeCategory == "" || writeMultiplier <= 0 || !IsValidCategory(writeCategory))
					continue;

				rpc.Write(writeCategory);
				rpc.Write(writeMultiplier);
			}
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

		// Rotation-Revision + Attachment-Aufschlaege
		WriteAttachmentSync(rpc, traderData);

		rpc.Send(player, SilverRPCManager.CHANNEL_SILVER_BARTER, true, player.GetIdentity());
		DebugLog("Menu RPC sent to " + player.GetIdentity().GetName());
	}

	void RpcRequestTraderMenuClose(ParamsReadContext ctx, PlayerIdentity sender)
	{
		if (!g_Game || !g_Game.IsDedicatedServer())
			return;

		int traderId;
		if (!ctx.Read(traderId))
		{
			Print("[SilverBarter] ERROR: Failed to read close traderId");
			return;
		}

		if (!sender)
			return;

		array<int> viewers;
		if (m_SilverBarter_OpenTraderMenus && m_SilverBarter_OpenTraderMenus.Find(traderId, viewers))
		{
			viewers.RemoveItem(sender.GetPlayerId());
			if (viewers.Count() == 0)
				m_SilverBarter_OpenTraderMenus.Remove(traderId);
		}
	}

	void RpcRequestTraderAction(ParamsReadContext ctx, PlayerIdentity sender)
	{
		if (!g_Game || !g_Game.IsDedicatedServer())
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

		int clientRotationRevision;
		if (!ctx.Read(clientRotationRevision))
		{
			Print("[SilverBarter] ERROR: Failed to read rotationRevision");
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

		// DIAGNOSE (temporaer): empfangene Buy-Mengen vor der Validierung
		for (int diagIdx = 0; diagIdx < buyItems.Count(); diagIdx++)
			DebugLog("Server empfangen Buy: " + buyItems.GetKey(diagIdx) + " qty=" + buyItems.GetElement(diagIdx).ToString());

		DebugLog("Trade: " + sellItems.Count().ToString() + " selling, " + buyItems.Count().ToString() + " buying");

		bool isRotatingTrade = IsRotatingTrader(traderId);
		SilverTrader_Info traderInfo;
		SilverTrader_Data traderData;

		if (isRotatingTrade)
		{
			SilverRotatingTrader_Config rotConfig;
			if (!m_SilverBarter_RotatingTraderCache.Find(traderId, rotConfig))
				return;
			traderInfo = rotConfig;
			if (!m_SilverBarter_RotatingTraderData.Find(traderId, traderData))
				return;
		}
		else
		{
			SilverTrader_ServerConfig stdConfig;
			if (!m_SilverBarter_TraderCache.Find(traderId, stdConfig))
				return;
			traderInfo = stdConfig;
			if (!m_SilverBarter_TraderData.Find(traderId, traderData))
				return;
		}

		// Rotations-Race: Client-Loadout muss dem aktuellen Server-Loadout entsprechen, sonst koennte der
		// Kaeufer ein anderes Attachment-Set erhalten als angezeigt. Bei Abweichung ablehnen und neu syncen.
		if (isRotatingTrade && traderData && traderData.m_rotationRevision != clientRotationRevision)
		{
			DebugLog("Trade denied: rotation revision mismatch (client=" + clientRotationRevision.ToString() + ", server=" + traderData.m_rotationRevision.ToString() + ")");
			SyncRotatingTraderToClients(traderId);
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
			if (!sellItem1 || sellItem1.IsPendingDeletion())
			{
				DebugLog("Sell rejected: null or pending deletion");
				continue;
			}

			string sellClass = sellItem1.GetType();
			float sellQty01 = CalculateItemQuantity01(sellItem1);
			DebugLog("Sell requested: " + sellClass + " qty01=" + sellQty01.ToString());

			if (!IsItemOwnedByPlayer(sellItem1, player))
			{
				DebugLog("Sell rejected: not owned by player - " + sellClass);
				continue;
			}
			if (!CanSellItem(traderInfo, sellItem1))
			{
				DebugLog("Sell rejected: CanSellItem false - " + sellClass);
				continue;
			}

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

		DebugLog("Sell requested/validated: " + sellItems.Count().ToString() + "/" + validSellItems.Count().ToString() + " - Buy requested/validated: " + buyItems.Count().ToString() + "/" + approvedBuyItems.Count().ToString());

		// Kein einseitiger Sell wenn alle Buy-Items serverseitig ungueltig waren
		if (validSellItems.Count() > 0 && approvedBuyItems.Count() == 0)
		{
			DebugLog("Trade denied: All buy items invalid after server validation for " + sender.GetName());
			return;
		}

		// === PHASE 3: Preis nur fuer validierte Items berechnen ===
		int resultPrice = 0;
		int totalSellValue = 0;

		foreach (ItemBase sellItem2 : validSellItems)
		{
			int sellPrice = CalculateSellPrice(traderInfo, traderData, sellItem2);
			DebugLog("Sell price: " + sellItem2.GetType() + " price=" + sellPrice.ToString());
			resultPrice = resultPrice + sellPrice;
			totalSellValue = totalSellValue + sellPrice;
		}

		foreach (string buyClassname2, float buyQuantity2 : approvedBuyItems)
		{
			int buyPrice = CalculateBuyPriceWithAttachments(traderInfo, traderData, buyClassname2, buyQuantity2);
			DebugLog("Buy price: " + buyClassname2 + " qty=" + buyQuantity2.ToString() + " price=" + buyPrice.ToString());
			resultPrice = resultPrice - buyPrice;
		}

		DebugLog("Result price: " + resultPrice.ToString());

		if (resultPrice < 0)
		{
			DebugLog("Trade denied: Negative price for " + sender.GetName());
			return;
		}

		// === PHASE 4: Trade ausfuehren ===
		// Kauf-Items werden zunaechst ausschliesslich in eine unerreichbare Staging-Chest gespawnt,
		// NIEMALS direkt ins Spielerinventar - verhindert, dass ein frisch gekauftes Item in einem
		// im selben Trade verkauften Container landet und mitgeloescht wird.
		vector deliveryPosition = player.GetPosition();
		string buyerUid = sender.GetId();

		// Chest MUSS innerhalb der Network-Bubble des Kaeufers erzeugt werden, sonst wird sie dem
		// Client nie repliziert und TakeEntityToInventory() schlaegt spaeter mit
		// "[syncinv] item not in bubble" fehl. Deshalb nur leicht unter der Oberflaeche versetzt.
		vector chestPos = Vector(deliveryPosition[0], deliveryPosition[1] - CHEST_STAGING_DEPTH, deliveryPosition[2]);
		SilverBarterChest chest = SilverBarterChest.Cast(g_Game.CreateObjectEx("SilverBarterChest", chestPos, ECE_NONE));
		if (!chest)
		{
			Print("[SilverBarter] ERROR: Could not create staging chest for trade, aborting.");
			return;
		}

		bool spawnFailed = false;
		foreach (string buyClassname4, float buyQuantity4 : approvedBuyItems)
		{
			if (spawnFailed)
				break;

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
				bool foundInvSlot = chest.GetInventory().FindFirstFreeLocationForNewEntity(buyClassname4, FindInventoryLocationType.ANY, invLoc);

				DebugSpawnInfo(buyClassname4);

				if (foundInvSlot)
				{
					buyEntity = ItemBase.Cast(chest.GetInventory().LocationCreateEntity(invLoc, buyClassname4, ECE_IN_INVENTORY, RF_DEFAULT));
					DebugLog("LocationCreateEntity result for " + buyClassname4 + ": " + (buyEntity != null).ToString() + " (foundInvSlot=true)");
				}

				// Fallback fuer Items die ECE_IN_INVENTORY nicht unterstuetzen (z.B. ItemBook-Subklassen)
				// Landet ebenfalls ausschliesslich in der Chest, niemals direkt beim Spieler
				if (!buyEntity)
				{
					ItemBase fallbackEntity = ItemBase.Cast(g_Game.CreateObject(buyClassname4, chestPos));
					if (fallbackEntity)
					{
						if (chest.GetInventory().TakeEntityToInventory(InventoryMode.SERVER, FindInventoryLocationType.ANY, fallbackEntity))
						{
							buyEntity = fallbackEntity;
						}
						else
						{
							// Darf niemals ausserhalb der Chest-Hierarchie zurueckbleiben
							g_Game.ObjectDelete(fallbackEntity);
						}
					}
					DebugLog("CreateObject fallback result for " + buyClassname4 + ": " + (buyEntity != null).ToString());
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
							// DIAGNOSE (temporaer): finaler AmmoCount nach dem Setzen
							DebugLog("Server nach Spawn: " + buyClassname4 + " spawnQuantity01=" + spawnQuantity01.ToString() + " finalAmmoCount=" + buyMagazine.GetAmmoCount().ToString());
						}
						else
						{
							buyMagazine.ServerSetAmmoCount(0);
						}
					}
					else if (GetOrCreateItemCache(buyClassname4).m_IsLiquidContainer)
					{
						buyEntity.SetQuantityNormalized(0);
					}
					else
					{
						buyEntity.SetQuantityNormalized(spawnQuantity01);
					}

					// Attachment-Baum je Waffe anhaengen (nur Rotating). Fehlschlag rollt ueber spawnFailed
					// die gesamte Chest-Hierarchie zurueck, damit Preis und geliefertes Item konsistent bleiben.
					if (isRotatingTrade)
					{
						SilverActivePoolEntry activeEntry = GetActivePoolEntry(traderId, buyClassname4);
						if (activeEntry && activeEntry.m_PoolItem && activeEntry.m_PoolItem.attachments && activeEntry.m_PoolItem.attachments.Count() > 0)
						{
							if (!SpawnAttachmentTree(buyEntity, activeEntry.m_PoolItem.attachments))
							{
								spawnFailed = true;
								break;
							}
						}
					}

					EntityAI spawnRoot = buyEntity.GetHierarchyRoot();
					string spawnRootType = "null";
					if (spawnRoot)
						spawnRootType = spawnRoot.GetType();
					DebugLog("SPAWN OK: " + buyClassname4 + " (qty=" + spawnQuantity01.ToString() + " root=" + spawnRootType + ")");
				}
				else
				{
					Print("[SilverBarter] SPAWN FAILED: " + buyClassname4 + " could not be created");
					spawnFailed = true;
					break;
				}

				calcQuantity = calcQuantity - 1;
			}

			if (spawnFailed)
				break;
		}

		// Bei Spawn-Fehler: nur die Chest loeschen, das loescht ihre komplette Cargo-Hierarchie
		// (alle bereits gespawnten Kauf-Items) automatisch mit
		if (spawnFailed)
		{
			Print("[SilverBarter] Trade aborted: spawn failure, rolling back for " + sender.GetName());
			g_Game.ObjectDelete(chest);
		}

		// Pruefen ob tatsaechlich etwas getauscht wurde
		bool tradeSuccess = (validSellItems.Count() > 0 || approvedBuyItems.Count() > 0) && !spawnFailed;

		if (!tradeSuccess)
		{
			DebugLog("Trade result: Nothing traded for " + sender.GetName());
		}

		// Nur bei erfolgreichem Spawn: Sell-Items loeschen und Trader-Bestand aendern
		if (tradeSuccess && traderData && traderData.m_items)
		{
			// Sell-Items ins Trader-Inventar aufnehmen
			foreach (ItemBase sellItem3 : validSellItems)
			{
				string sellClassname = sellItem3.GetType();

				if (isRotatingTrade)
				{
					DebugLog("Rotating Trader " + traderId.ToString() + " destroys sold item: " + sellClassname);
					continue;
				}

				float maxQuantity = CalculateTraderItemQuantityMax(traderInfo, sellClassname);
				float itemQuantity = CalculateItemQuantity01(sellItem3);
				float newValue = 0;

				if (traderData.m_items.Contains(sellClassname))
				{
					newValue = Math.Min(maxQuantity, traderData.m_items.Get(sellClassname) + itemQuantity);
					traderData.m_items.Set(sellClassname, newValue);
				}
				else
				{
					newValue = Math.Min(maxQuantity, itemQuantity);
					traderData.m_items.Set(sellClassname, newValue);
				}

				DebugLog("Trader " + traderId.ToString() + " buys: " + sellClassname);
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

			// Verkaufte Items vom Spieler loeschen (rueckwaerts, Attachments vor Container)
			for (int i = validSellItems.Count() - 1; i >= 0; i--)
			{
				ItemBase sellItem4 = validSellItems[i];
				if (sellItem4 && !sellItem4.IsPendingDeletion() && IsItemOwnedByPlayer(sellItem4, player))
				{
					g_Game.ObjectDelete(sellItem4);
				}
			}
		}

		#ifdef ZenSkills
		if (tradeSuccess && totalSellValue > 0 && player && SilverBarterConfigService.GetConfig().m_zenSkillsXPEnabled)
		{
			int earnedEXP = Math.Min(Math.Floor(totalSellValue / 100.0), 25);

			if (earnedEXP > 0)
				player.AddZenSkillEXP("gathering", earnedEXP);
		}
		#endif

		// Chest-Auslieferung: kein sofortiger Zustellversuch direkt nach dem Loeschen der Sell-Items -
		// FindInventoryLocationType.ANY koennte sonst einen Slot in einem noch nicht vollstaendig
		// entfernten (aber schon zur Loeschung vorgemerkten) Container waehlen. Stattdessen genau
		// EIN verzoegerter Zustellversuch per CallLater.
		if (!spawnFailed)
		{
			if (tradeSuccess)
			{
				g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.FinishDelivery, 200, false, chest, buyerUid, deliveryPosition, traderId);
			}
			else
			{
				// Nichts getauscht - Chest wurde erzeugt, blieb aber leer
				g_Game.ObjectDelete(chest);
			}
		}

		// Trader als dirty markieren (nur normale Trader persistent speichern)
		if (tradeSuccess && !isRotatingTrade)
		{
			m_SilverBarter_DirtyTraders.Insert(traderId);
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

			// Rotation-Revision + Attachment-Aufschlaege
			WriteAttachmentSync(respRpc, traderData);

			respRpc.Send(respPlayer, SilverRPCManager.CHANNEL_SILVER_BARTER, true, sender);
			DebugLog("Trade response sent: success=" + tradeSuccess.ToString() + ", items=" + respItemCount.ToString());
		}
	}

	// Stufe 1: Einstieg in die serielle Zustellung. Der 200ms-Delay vor diesem Callback gibt der
	// Engine Zeit, Chest + Items dem Kaeufer-Client zu replizieren, bevor der erste Inventory-Move
	// angestossen wird. Bewusstes kleines Restrisiko: die Chest ist als normale replizierte SeaChest
	// fuer dieses Zeitfenster potenziell sicht-/zugreifbar - keine neue Ownership-Architektur dafuer.
	void FinishDelivery(SilverBarterChest chest, string buyerUid, vector deliveryPosition, int traderId)
	{
		if (!chest || chest.IsPendingDeletion())
			return;

		PlayerBase buyer = GetPlayerByUid(buyerUid);
		if (buyer)
			DebugLog("FinishDelivery distance: " + vector.Distance(chest.GetPosition(), buyer.GetPosition()).ToString() + " chestPos=" + chest.GetPosition().ToString() + " buyerPos=" + buyer.GetPosition().ToString());

		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.DeliverNext, DELIVERY_POLL_INTERVAL_MS, false, chest, buyerUid, deliveryPosition, traderId, null, 0, 0);
	}

	// Stufe 2: Serielle Zustellung - pro Tick laeuft hoechstens EINE Inventaroperation (Take ODER Drop).
	// Mehrere gleichzeitige TakeEntityToInventory()-Moves auf denselben Spieler kollidieren ueber
	// Junctures/Reservations, sodass nur der erste greift. Deshalb wird immer nur ein Item in Zustellung
	// gehalten (inFlightItem) und das naechste erst angestossen, wenn das vorige die Chest verlassen hat.
	void DeliverNext(SilverBarterChest chest, string buyerUid, vector deliveryPosition, int traderId, EntityAI inFlightItem, int inFlightPolls, int globalPolls)
	{
		if (!chest || chest.IsPendingDeletion())
			return;

		CargoBase cargo = chest.GetInventory().GetCargo();
		int remaining = 0;
		if (cargo)
			remaining = cargo.GetItemCount();

		// Alles zugestellt
		if (remaining == 0)
		{
			DebugLog("DeliverNext: chest empty after " + globalPolls.ToString() + " polls, delivery ok for trader " + traderId.ToString());
			FinalizeChest(chest, deliveryPosition, traderId, buyerUid);
			return;
		}

		// Globales Sicherheitsnetz - greift im Normalfall nie (Sub-Timeout leert die Chest vorher)
		if (globalPolls >= DELIVERY_MAX_POLLS)
		{
			DebugLog("DeliverNext GLOBAL TIMEOUT: " + remaining.ToString() + " item(s) forced to ground for trader " + traderId.ToString());
			DropAllChestItems(chest, deliveryPosition);
			FinalizeChest(chest, deliveryPosition, traderId, buyerUid);
			return;
		}

		// In-Flight-Auswertung: hat das zuletzt angestossene Item die Chest verlassen?
		if (inFlightItem)
		{
			if (inFlightItem.GetHierarchyParent() == chest)
			{
				// Noch in der Chest: Move laeuft noch oder ist gescheitert
				if (inFlightPolls >= DELIVERY_ITEM_MAX_POLLS)
				{
					// Als gescheitert werten - Boden-Fallback, dann diesen Tick beenden (nur EINE Operation/Tick)
					if (!DropDeliveryItem(chest, ItemBase.Cast(inFlightItem), deliveryPosition))
					{
						FinalizeChest(chest, deliveryPosition, traderId, buyerUid);
						return;
					}
					g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.DeliverNext, DELIVERY_POLL_INTERVAL_MS, false, chest, buyerUid, deliveryPosition, traderId, null, 0, globalPolls + 1);
					return;
				}

				// Weiter warten - kein neuer Move auf demselben Item
				g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.DeliverNext, DELIVERY_POLL_INTERVAL_MS, false, chest, buyerUid, deliveryPosition, traderId, inFlightItem, inFlightPolls + 1, globalPolls + 1);
				return;
			}

			// Erfolgreich verschwunden - abhaken, im selben Tick darf das naechste starten (vorige Operation ist fertig)
			inFlightItem = null;
			inFlightPolls = 0;
		}

		// Naechstes Item anstossen (genau eines)
		ItemBase head = null;
		if (cargo && cargo.GetItemCount() > 0)
			head = ItemBase.Cast(cargo.GetItem(0));

		if (head)
		{
			PlayerBase buyer = GetPlayerByUid(buyerUid);
			bool takeResult = false;
			if (buyer)
				takeResult = buyer.GetInventory().TakeEntityToInventory(InventoryMode.SERVER, FindInventoryLocationType.ANY, head);

			if (takeResult)
			{
				DebugLog("DeliverNext move started: " + head.GetType());
				g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.DeliverNext, DELIVERY_POLL_INTERVAL_MS, false, chest, buyerUid, deliveryPosition, traderId, head, 0, globalPolls + 1);
				return;
			}

			// Move gar nicht angenommen (kein Buyer / kein Platz) - Boden-Fallback, dann Tick beenden
			if (!DropDeliveryItem(chest, head, deliveryPosition))
			{
				FinalizeChest(chest, deliveryPosition, traderId, buyerUid);
				return;
			}
		}

		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.DeliverNext, DELIVERY_POLL_INTERVAL_MS, false, chest, buyerUid, deliveryPosition, traderId, null, 0, globalPolls + 1);
	}

	// Legt alle verbliebenen Chest-Items auf den Boden (stabile Iteration ueber eine Kopie).
	private void DropAllChestItems(SilverBarterChest chest, vector deliveryPosition)
	{
		if (!chest)
			return;

		CargoBase cargo = chest.GetInventory().GetCargo();
		if (!cargo)
			return;

		array<ItemBase> items = new array<ItemBase>;
		for (int i = cargo.GetItemCount() - 1; i >= 0; i--)
		{
			ItemBase item = ItemBase.Cast(cargo.GetItem(i));
			if (item)
				items.Insert(item);
		}

		foreach (ItemBase dropItem : items)
		{
			if (dropItem && dropItem.GetHierarchyParent() == chest)
				DropDeliveryItem(chest, dropItem, deliveryPosition);
		}
	}

	// Chest abschliessen: leer -> loeschen; nicht leer -> niemals mit Inhalt loeschen, stattdessen
	// sichtbar zum Spieler bringen und Warnung loggen. In jedem Fall wird der Client ueber das Ende
	// der Zustellung informiert, damit er die Sell-Liste aktualisieren kann.
	private void FinalizeChest(SilverBarterChest chest, vector deliveryPosition, int traderId, string buyerUid)
	{
		if (!chest || chest.IsPendingDeletion())
			return;

		CargoBase cargo = chest.GetInventory().GetCargo();
		int remaining = 0;
		if (cargo)
			remaining = cargo.GetItemCount();

		if (remaining == 0)
		{
			g_Game.ObjectDelete(chest);
			NotifyDeliveryComplete(buyerUid);
			return;
		}

		chest.SetPosition(deliveryPosition);
		Print("[SilverBarter] WARNING: " + remaining.ToString() + " item(s) could not be delivered for trader " + traderId.ToString() + ", chest moved to delivery position instead of deleted.");
		NotifyDeliveryComplete(buyerUid);
	}

	// Server: dem Kaeufer melden, dass die serielle Zustellung abgeschlossen ist. Traegt keinen Payload -
	// der Client refresht daraufhin (mit kleinem Sync-Puffer) seine Sell-Liste. Menue zu -> egal, RPC laeuft
	// clientseitig einfach ins Leere.
	private void NotifyDeliveryComplete(string buyerUid)
	{
		PlayerBase buyer = GetPlayerByUid(buyerUid);
		if (!buyer)
			return;

		PlayerIdentity identity = buyer.GetIdentity();
		if (!identity)
			return;

		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(SilverRPC.SILVERRPC_DELIVERY_COMPLETE);
		rpc.Send(buyer, SilverRPCManager.CHANNEL_SILVER_BARTER, true, identity);
	}

	private bool DropDeliveryItem(SilverBarterChest chest, ItemBase item, vector deliveryPosition)
	{
		if (!chest || !item || item.GetHierarchyParent() != chest)
			return false;

		vector transform[4];
		Math3D.MatrixIdentity4(transform);
		transform[3] = deliveryPosition;
		bool dropped = chest.GetInventory().DropEntityWithTransform(InventoryMode.SERVER, chest, item, transform);
		DebugLog("DropDeliveryItem: " + item.GetType() + " dropped=" + dropped.ToString());
		return dropped;
	}

	private PlayerBase GetPlayerByUid(string uid)
	{
		array<Man> players = new array<Man>;
		g_Game.GetPlayers(players);
		foreach (Man man : players)
		{
			PlayerBase pb = PlayerBase.Cast(man);
			if (pb && pb.GetIdentity() && pb.GetIdentity().GetId() == uid)
				return pb;
		}
		return null;
	}

	void SaveTraderData(int traderId)
	{
		SilverTrader_Data traderData;
		if (m_SilverBarter_TraderData.Find(traderId, traderData))
		{
			string dataPath = DATA_FOLDER + "trader_" + traderId.ToString() + ".json";
			traderData.SaveToJson(dataPath);
		}
	}

	void SaveDirtyTraderData()
	{
		if (!g_Game || !g_Game.IsDedicatedServer())
			return;

		if (!m_SilverBarter_DirtyTraders || m_SilverBarter_DirtyTraders.Count() == 0)
			return;

		foreach (int traderId : m_SilverBarter_DirtyTraders)
		{
			SaveTraderData(traderId);
		}

		DebugLog(m_SilverBarter_DirtyTraders.Count().ToString() + " trader data saved.");

		m_SilverBarter_DirtyTraders.Clear();
	}

	void SaveAllTraderData()
	{
		if (!g_Game || !g_Game.IsDedicatedServer())
			return;

		if (!m_SilverBarter_TraderData)
			return;

		foreach (int traderId, SilverTrader_Data data : m_SilverBarter_TraderData)
		{
			SaveTraderData(traderId);
		}

		if (m_SilverBarter_DirtyTraders)
		{
			m_SilverBarter_DirtyTraders.Clear();
		}

		DebugLog("All trader data saved (shutdown).");
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
		super.OnUpdate(delta_time);

		if (!g_Game || !g_Game.IsDedicatedServer())
			return;

		// Periodisches Speichern (nur dirty Trader)
		m_SilverBarter_SaveTimer = m_SilverBarter_SaveTimer + delta_time;
		if (m_SilverBarter_SaveTimer > SAVE_INTERVAL)
		{
			m_SilverBarter_SaveTimer = 0;
			SaveDirtyTraderData();
		}

		// Rotierende Haendler Timer pruefen
		CheckRotationTimers(delta_time);

		// ZenMap Marker verzoegert setzen (warten bis ZenMap-Plugin bereit ist)
		#ifdef ZenMap
		if (!m_SilverBarter_ZenMapMarkersSet && m_SilverBarter_RotatingTraderCache)
		{
			PluginZenMapMarkers zenCheck = PluginZenMapMarkers.Cast(GetPlugin(PluginZenMapMarkers));
			if (zenCheck)
			{
				foreach (int zenId, SilverRotatingTrader_Config zenCfg : m_SilverBarter_RotatingTraderCache)
				{
					SetZenMapMarker(zenCfg);
				}
				m_SilverBarter_ZenMapMarkersSet = true;
				DebugLog("ZenMap Marker fuer rotierende Haendler gesetzt.");
			}
		}
		#endif
	}

	override void OnDestroy()
	{
		CGame game = g_Game;
		if (game)
		{
			game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.FinishDelivery);
			game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.DeliverNext);
		}

		SilverRPCManager.UnregisterInstance(this);
		SaveAllTraderData();

		if (game && m_SilverBarter_TraderPoints)
		{
			foreach (int id, TraderPoint obj : m_SilverBarter_TraderPoints)
			{
				if (obj)
					game.ObjectDelete(obj);
			}
		}

		// ZenMap Marker entfernen
		#ifdef ZenMap
		if (m_SilverBarter_RotatingTraderCache)
		{
			foreach (int zenTraderId, SilverRotatingTrader_Config zenConfig : m_SilverBarter_RotatingTraderCache)
			{
				RemoveZenMapMarker(zenConfig);
			}
		}
		#endif

		if (game && m_SilverBarter_RotatingTraderPoints)
		{
			foreach (int rotId, TraderPoint rotObj : m_SilverBarter_RotatingTraderPoints)
			{
				if (rotObj)
					game.ObjectDelete(rotObj);
			}
		}

		super.OnDestroy();
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
		if (data && data.m_items && data.m_items.Contains(classname))
		{
			totalQuantity = data.m_items.Get(classname);
		}

		return CalculateBuyPriceAtStock(trader, classname, quantity, totalQuantity);
	}

	// Kern der Kaufpreis-Berechnung mit explizitem Lagerbestand. Fuer Attachments mit totalQuantity 0 aufrufen:
	// dann liefert das Dumping den vollen Preis, unabhaengig von einem separaten Stock desselben classname.
	int CalculateBuyPriceAtStock(SilverTrader_Info trader, string classname, float quantity, float totalQuantity)
	{
		float itemMaxQuantity = CalculateTraderItemQuantityMax(trader, classname);
		quantity = Math.Min(quantity, itemMaxQuantity);
		totalQuantity = Math.Min(totalQuantity, itemMaxQuantity);

		float basePrice = CalculateDumping(trader.m_dumpingByAmountAlgorithm, trader.m_dumpingByAmountModifier, (int)totalQuantity, (int)itemMaxQuantity);
		basePrice = basePrice * quantity * 1000;

		float resultPrice = basePrice * GetCategoryValueMultiplier(trader, classname);

		if (resultPrice < 1)
			return 1;

		return (int)Math.Floor(resultPrice);
	}

	// Stueckpreis eines einzelnen Attachments: voller Preis bei Lagerbestand 0 (kein Dumping-Rabatt).
	int CalculateAttachmentUnitPrice(SilverTrader_Info trader, string classname)
	{
		return CalculateBuyPriceAtStock(trader, classname, 1, 0);
	}

	// Summe der Aufschlaege eines Attachment-Baums. Jeder Knoten wird pro Stueck gefloort und aufaddiert,
	// damit Client-Anzeige und Server-Abrechnung bit-identisch bleiben. Bewusst iterativ ueber eine Queue statt
	// rekursiv - ein rekursiver Aufruf im Schleifenkoerper zerstoert in diesem Enforce-Build den Schleifenzustand.
	int CalculateAttachmentTreeSurcharge(SilverTrader_Info trader, array<ref SilverAttachmentSpec> rootSpecs)
	{
		if (!rootSpecs)
			return 0;

		int sum = 0;
		array<ref SilverAttachJob> queue = new array<ref SilverAttachJob>;
		SilverAttachJob rootJob = new SilverAttachJob();
		rootJob.m_Specs = rootSpecs;
		queue.Insert(rootJob);

		while (queue.Count() > 0)
		{
			SilverAttachJob job = queue.Get(0);
			queue.Remove(0);
			if (!job || !job.m_Specs)
				continue;

			array<ref SilverAttachmentSpec> specs = job.m_Specs;
			int count = specs.Count();
			for (int i = 0; i < count; i++)
			{
				SilverAttachmentSpec spec = specs.Get(i);
				if (!spec || spec.classname == "")
					continue;

				sum = sum + CalculateAttachmentUnitPrice(trader, spec.classname);

				if (spec.attachments && spec.attachments.Count() > 0)
				{
					SilverAttachJob childJob = new SilverAttachJob();
					childJob.m_Specs = spec.attachments;
					queue.Insert(childJob);
				}
			}
		}
		return sum;
	}

	// Baut die flache Preview-Liste (classname, slot, parentIndex) eines Attachment-Baums fuer den Client-Sync.
	// BFS ueber eine Queue: der Parent-Knoten wird stets vor seinen Kindern eingefuegt, daher parentIndex < eigener Index.
	array<ref SilverPreviewAttachment> BuildPreviewList(array<ref SilverAttachmentSpec> rootSpecs)
	{
		array<ref SilverPreviewAttachment> flat = new array<ref SilverPreviewAttachment>;
		if (!rootSpecs || rootSpecs.Count() == 0)
			return flat;

		array<ref SilverAttachJob> queue = new array<ref SilverAttachJob>;
		SilverAttachJob rootJob = new SilverAttachJob();
		rootJob.m_Specs = rootSpecs;
		rootJob.m_ParentIndex = -1;
		queue.Insert(rootJob);

		while (queue.Count() > 0)
		{
			SilverAttachJob job = queue.Get(0);
			queue.Remove(0);
			if (!job || !job.m_Specs)
				continue;

			array<ref SilverAttachmentSpec> specs = job.m_Specs;
			int parentIndex = job.m_ParentIndex;
			int count = specs.Count();
			for (int i = 0; i < count; i++)
			{
				SilverAttachmentSpec spec = specs.Get(i);
				if (!spec || spec.classname == "")
					continue;

				SilverPreviewAttachment preview = new SilverPreviewAttachment();
				preview.m_Classname = spec.classname;
				preview.m_Slot = spec.slot;
				preview.m_ParentIndex = parentIndex;
				int myIndex = flat.Count();
				flat.Insert(preview);

				if (spec.attachments && spec.attachments.Count() > 0)
				{
					SilverAttachJob childJob = new SilverAttachJob();
					childJob.m_Specs = spec.attachments;
					childJob.m_ParentIndex = myIndex;
					queue.Insert(childJob);
				}
			}
		}
		return flat;
	}

	// Eingefrorener Attachment-Stueckaufschlag fuer classname aus den Trader-Daten (0 wenn keine Attachments aktiv).
	int GetAttachmentSurcharge(SilverTrader_Data data, string classname)
	{
		if (!data || !data.m_attachmentSurcharge)
			return 0;

		int surcharge = 0;
		if (data.m_attachmentSurcharge.Find(classname, surcharge))
			return surcharge;
		return 0;
	}

	// Kaufpreis inkl. Attachment-Aufschlag. Aufschlag gilt pro Waffe, daher * quantity.
	// Wird von Client (Anzeige) und Server (Abrechnung) gleichermassen genutzt.
	int CalculateBuyPriceWithAttachments(SilverTrader_Info trader, SilverTrader_Data data, string classname, float quantity)
	{
		int basePrice = CalculateBuyPrice(trader, data, classname, quantity);
		int surcharge = GetAttachmentSurcharge(data, classname);
		if (surcharge <= 0)
			return basePrice;

		return basePrice + (int)(surcharge * quantity);
	}

	// Wert-Multiplikator fuer die Item-Kategorie: zuerst trader-spezifisch, danach global (Server-Config bzw. client-gesyncte Liste)
	float GetCategoryValueMultiplier(SilverTrader_Info trader, string classname)
	{
		string category = GetOrCreateItemCache(classname).m_Category;
		if (category == "" || !IsValidCategory(category))
			return 1.0;

		if (trader && trader.m_categoryValueMultipliers)
		{
			float traderMultiplier;
			if (trader.m_categoryValueMultipliers.Find(category, traderMultiplier) && traderMultiplier > 0)
				return traderMultiplier;
		}

		if (m_SilverBarter_Config && m_SilverBarter_Config.m_categoryValueMultipliers)
		{
			float serverMultiplier;
			if (m_SilverBarter_Config.m_categoryValueMultipliers.Find(category, serverMultiplier) && serverMultiplier > 0)
				return serverMultiplier;
		}
		else if (m_SilverBarter_CategoryValueMultipliersClient)
		{
			float clientMultiplier;
			if (m_SilverBarter_CategoryValueMultipliersClient.Find(category, clientMultiplier) && clientMultiplier > 0)
				return clientMultiplier;
		}

		return 1.0;
	}

	// Kategorie-Override fuer classname suchen (Server-Config wenn aktiv, sonst client-gesyncte Liste)
	private string GetCategoryOverride(string classname)
	{
		if (m_SilverBarter_CategoryOverridesConfig && m_SilverBarter_CategoryOverridesConfig.m_enabled && m_SilverBarter_CategoryOverridesConfig.m_categoryOverrides)
		{
			foreach (SilverCategoryOverride ovServer : m_SilverBarter_CategoryOverridesConfig.m_categoryOverrides)
			{
				if (MatchCategoryOverride(classname, ovServer))
				{
					string serverCategory = ovServer.category;
					serverCategory.ToLower();
					return serverCategory;
				}
			}
		}
		else if (m_SilverBarter_CategoryOverridesClient)
		{
			foreach (SilverCategoryOverride ovClient : m_SilverBarter_CategoryOverridesClient)
			{
				if (MatchCategoryOverride(classname, ovClient))
				{
					string clientCategory = ovClient.category;
					clientCategory.ToLower();
					return clientCategory;
				}
			}
		}

		return "";
	}

	private bool MatchCategoryOverride(string classname, SilverCategoryOverride ov)
	{
		if (!ov || ov.pattern == "" || ov.category == "")
			return false;

		string ovCategory = ov.category;
		ovCategory.ToLower();

		if (!IsValidCategory(ovCategory))
			return false;

		if (ov.prefixOnly)
			return classname.IndexOf(ov.pattern) == 0;

		return classname == ov.pattern;
	}

	private bool IsValidCategory(string category)
	{
		category.ToLower();
		return s_ValidCategories && s_ValidCategories.Find(category) >= 0;
	}

	SilverItemConfigCache GetOrCreateItemCache(string classname)
	{
		SilverItemConfigCache cache;
		if (m_SilverBarter_ItemConfigCache && m_SilverBarter_ItemConfigCache.Find(classname, cache))
			return cache;

		cache = new SilverItemConfigCache();
		cache.m_ItemCapacity     = 1;
		cache.m_IsLiquidContainer = false;
		cache.m_MaxStackSize     = 0;
		cache.m_StackedUnit      = "";
		cache.m_IsAmmo           = false;
		cache.m_CanBeSplit       = false;

		string cfgRoot = "";
		if (g_Game.ConfigIsExisting(CFG_VEHICLESPATH + " " + classname))
			cfgRoot = CFG_VEHICLESPATH;
		else if (g_Game.ConfigIsExisting(CFG_MAGAZINESPATH + " " + classname))
			cfgRoot = CFG_MAGAZINESPATH;
		else if (g_Game.ConfigIsExisting(CFG_WEAPONSPATH + " " + classname))
			cfgRoot = CFG_WEAPONSPATH;

		if (cfgRoot != "")
		{
			string base = cfgRoot + " " + classname;

			vector itemSize = "1 1 0";
			if (g_Game.ConfigIsExisting(base + " itemSize"))
				itemSize = g_Game.ConfigGetVector(base + " itemSize");
			cache.m_ItemCapacity = (int)Math.Max(1, itemSize[0] * itemSize[1]);

			cache.m_IsLiquidContainer = g_Game.ConfigIsExisting(base + " liquidContainerType");

			if (cfgRoot == CFG_MAGAZINESPATH)
			{
				if (g_Game.ConfigIsExisting(base + " count"))
					cache.m_MaxStackSize = g_Game.ConfigGetInt(base + " count");
			}
			else
			{
				if (g_Game.ConfigIsExisting(base + " varQuantityMax"))
					cache.m_MaxStackSize = g_Game.ConfigGetInt(base + " varQuantityMax");
			}

			if (g_Game.ConfigIsExisting(base + " stackedUnit"))
				cache.m_StackedUnit = g_Game.ConfigGetTextOut(base + " stackedUnit");

			if (cfgRoot == CFG_VEHICLESPATH && g_Game.ConfigIsExisting(base + " canBeSplit"))
				cache.m_CanBeSplit = g_Game.ConfigGetInt(base + " canBeSplit") == 1;
		}

		cache.m_IsAmmo = g_Game.IsKindOf(classname, "Ammunition_Base");

		// Kategorie fuer FilterByCategories einmalig bestimmen
		string cat = "other";

		if (g_Game.IsKindOf(classname, "Grenade_Base") || cfgRoot == CFG_WEAPONSPATH)
		{
			cat = "weapons";
		}
		else if (cache.m_IsAmmo || classname.IndexOf("AmmoBox") == 0)
		{
			cat = "ammo";
		}
		else if (g_Game.IsKindOf(classname, "Magazine_Base"))
		{
			cat = "magazines";
		}
		else if (s_ToolClasses)
		{
			foreach (string tc : s_ToolClasses)
			{
				if (g_Game.IsKindOf(classname, tc))
				{
					cat = "tools";
					break;
				}
			}
		}

		if (cat == "other" && cfgRoot == CFG_VEHICLESPATH)
		{
			string vBase = CFG_VEHICLESPATH + " " + classname;

			if (g_Game.ConfigIsExisting(vBase + " vehiclePartItem") && g_Game.ConfigGetInt(vBase + " vehiclePartItem") == 1)
			{
				cat = "vehicle_parts";
			}
			else if (g_Game.IsKindOf(classname, "BaseBuildingBase") || g_Game.IsKindOf(classname, "Container_Base")	|| (g_Game.ConfigIsExisting(vBase + " baseBuildingItem") && g_Game.ConfigGetInt(vBase + " baseBuildingItem") == 1))
			{
				cat = "base_building";
			}
			else if (g_Game.ConfigIsExisting(vBase + " inventorySlot"))
			{
				TStringArray invSlots = new TStringArray;
				g_Game.ConfigGetTextArray(vBase + " inventorySlot", invSlots);
				foreach (string slot : invSlots)
				{
					slot.ToLower();
					if (slot.IndexOf("weapon") == 0) { cat = "attachments"; break; }
					else if (slot == "melee") { cat = "tools"; break; }
				}
			}

			if (cat == "other" && g_Game.ConfigIsExisting(vBase + " attachments"))
			{
				TStringArray atts = new TStringArray;
				g_Game.ConfigGetTextArray(vBase + " attachments", atts);
				foreach (string att : atts)
				{
					att.ToLower();
					if (att.IndexOf("batteryd") == 0) { cat = "electronic"; break; }
				}
			}

			if (cat == "other" && g_Game.ConfigIsExisting(vBase + " medicalItem") && g_Game.ConfigGetInt(vBase + " medicalItem") == 1)
				cat = "medical";
		}

		if (cat == "other")
		{
			if (g_Game.IsKindOf(classname, "Edible_Base"))
				cat = "food";
			else if (g_Game.IsKindOf(classname, "Clothing_Base"))
				cat = "clothing";
		}

		string overrideCategory = GetCategoryOverride(classname);
		if (overrideCategory != "")
			cat = overrideCategory;

		cat.ToLower();
		cache.m_Category = cat;

		if (!m_SilverBarter_ItemConfigCache)
			m_SilverBarter_ItemConfigCache = new map<string, ref SilverItemConfigCache>;
		m_SilverBarter_ItemConfigCache.Insert(classname, cache);
		return cache;
	}

	float CalculateTraderItemQuantityMax(SilverTrader_Info trader, string classname)
	{
		SilverItemConfigCache cache = GetOrCreateItemCache(classname);
		return Math.Round(((float)trader.m_storageMaxSize) / cache.m_ItemCapacity);
	}

	float CalculateItemQuantity01(ItemBase item)
	{
		if (item.GetLiquidTypeInit() != 0)
		{
			if (IsQuantityPriceItem(item.GetType()))
			{
				float liqQty = item.GetQuantity();
				int liqMax = item.GetQuantityMax();
				if (liqMax > 0)
					return Math.Min(liqQty, liqMax) / (float)liqMax;
			}
			return 1;
		}

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

	bool IsQuantityPriceItem(string classname)
	{
		if (m_SilverBarter_Config && m_SilverBarter_Config.m_quantityPriceClassnames)
		{
			foreach (string qpClass : m_SilverBarter_Config.m_quantityPriceClassnames)
			{
				if (g_Game.IsKindOf(classname, qpClass))
					return true;
			}
		}

		if (m_SilverBarter_QuantityPriceClassnamesClient)
		{
			foreach (string qpClass2 : m_SilverBarter_QuantityPriceClassnamesClient)
			{
				if (g_Game.IsKindOf(classname, qpClass2))
					return true;
			}
		}

		return false;
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
		SilverItemConfigCache cache = GetOrCreateItemCache(classname);

		if (cache.m_IsLiquidContainer)
			return 1;

		if (cache.m_MaxStackSize > 0 && (cache.m_StackedUnit == "pc." || cache.m_IsAmmo))
			return 1.0 / cache.m_MaxStackSize;

		return 1;
	}

	float CalculateDumping(string algorithm, float modifier, float value, float max)
	{
		return Math.Lerp(1, modifier, (value / max));
	}

	void DoBarter(int traderId, array<ItemBase> sellItems, map<string, float> buyItems, int rotationRevision)
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
		rpc.Write(rotationRevision);

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
		SilverItemConfigCache cache = GetOrCreateItemCache(classname);
		return IsCategoryEnabled(categories, enabledCategories, cache.m_Category);
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
		if (m_SilverBarter_Config && m_SilverBarter_Config.m_debugMode)
		{
			Print("[SilverBarter] " + message);
		}
	}

	void DebugSpawnInfo(string cn)
	{
		string p = "";
		if (g_Game.ConfigIsExisting("CfgVehicles " + cn))
			p = "CfgVehicles " + cn;
		else if (g_Game.ConfigIsExisting("CfgMagazines " + cn))
			p = "CfgMagazines " + cn;
		else if (g_Game.ConfigIsExisting("CfgWeapons " + cn))
			p = "CfgWeapons " + cn;

		if (p == "")
		{
			DebugLog("SPAWN DEBUG: " + cn + " not found in any CfgPath on server");
			return;
		}

		int scope = 0;
		if (g_Game.ConfigIsExisting(p + " scope"))
			scope = g_Game.ConfigGetInt(p + " scope");

		DebugLog("SPAWN DEBUG: " + cn + " path=" + p + " scope=" + scope.ToString());
	}
};
