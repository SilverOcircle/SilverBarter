// SilverBarter Trader-Menu UI
class SilverTraderMenu extends UIScriptedMenu
{
	const float SELL_ITEM_DEPTH_OFFSET = 30;
	const float SELL_ITEM_HEIGHT_OFFSET = 2;
	const float PROGRESS_BAR_PRICE_DIVIDER = 25;
	const int PREVIEW_POOL_CAP = 100;

	// TradeButton-Farbe wird per Script gesetzt statt dem Style-Disabled-Zustand ueberlassen -
	// DayZDefaultButtonAll zeigt den explizit gesetzten "color"-Wert unabhaengig vom Enable()-Zustand an
	const int TRADE_BUTTON_COLOR_ENABLED = 0xFF1E6A25;
	const int TRADE_BUTTON_COLOR_DISABLED = 0xFF3C3C3C;

	// Test-Layout unter layout/test/ statt der normalen Layouts laden - vor Release wieder auf false
	const bool DEBUG_USE_TEST_LAYOUT = true;

	bool m_SilverBarter_Active = false;
	bool m_SilverBarter_Dirty = false;

	int m_SilverBarter_TraderId;
	bool m_SilverBarter_IsRotatingTrader;
	ref SilverTrader_Info m_SilverBarter_TraderInfo;
	ref SilverTrader_Data m_SilverBarter_TraderData;

	ref ScrollWidget m_SilverBarter_SellItemsPanel;
	ref ScrollWidget m_SilverBarter_BuyItemsPanel;

	ref SimpleProgressBarWidget m_SilverBarter_ProgressPositive;
	ref SimpleProgressBarWidget m_SilverBarter_ProgressNegative;

	ref ButtonWidget m_SilverBarter_BarterButton;
	ref MultilineTextWidget m_SilverBarter_TradeButtonInfo;

	ref array<ref Widget> m_SilverBarter_SellWidgetsCache;
	ref array<ref Widget> m_SilverBarter_BuyWidgetsCache;
	ref array<ref SilverTraderMenuBuyData> m_SilverBarter_BuyData;

	ref EditBoxWidget m_SilverBarter_BuySearchBox;
	ref EditBoxWidget m_SilverBarter_SellSearchBox;
	string m_SilverBarter_BuySearchText = "";
	string m_SilverBarter_SellSearchText = "";
	// Debounce fuer Sell-Suche - rekursiver Inventar-Rebuild ist teurer als der Buy-Rebuild
	float m_SilverBarter_PendingSellSearchTimer = -1;
	// Debounce fuer Buy-Suche - vermeidet Sort() ueber m_SilverBarter_TraderData.m_items bei jedem Tastendruck
	float m_SilverBarter_PendingBuySearchTimer = -1;

	// Persistente Kauf-Auswahl (Classname -> gewaehlte Menge), uebersteht Filter-Toggles und Rebuilds
	ref map<string, float> m_SilverBarter_BuySelectedQuantities;

	// Lazy-Preview: Pool (classname→Entity) + aktive Zuordnung (index→Entity)
	ref map<string, EntityAI> m_SilverBarter_PreviewPool;
	ref map<int, EntityAI> m_SilverBarter_PreviewByIndex;
	float m_SilverBarter_BuyRowHeight = 0;
	float m_SilverBarter_BuyPanelHeight = 0;
	float m_SilverBarter_LastScrollPos01 = -1;

	ref array<string> m_SilverBarter_FilterData;
	static ref array<bool> s_SilverBarter_FilterMemory;

	ref map<string, string> m_SilverBarter_DisplayNameCache;

	float m_SilverBarter_CurrentBarterProgress = 0;
	bool m_SilverBarter_BlockBarter = true;

	// Einmaliger verzoegerter Sell-Rebuild nach Trade-Erfolg, bis frisch gespawnte Items ihre echte Menge repliziert haben
	float m_SilverBarter_PendingSellRefreshTimer = -1;

	// Batched Build
	const int BUILD_BATCH_SIZE = 20;
	ref array<string> m_SilverBarter_PendingBuyClassnames;
	ref array<float> m_SilverBarter_PendingBuyQuantities;
	int m_SilverBarter_BuildIndex = 0;
	int m_SilverBarter_CachedScreenHeight = 0;

	void SilverTraderMenu()
	{
		m_SilverBarter_SellWidgetsCache = new array<ref Widget>;
		m_SilverBarter_BuyWidgetsCache = new array<ref Widget>;
		m_SilverBarter_BuyData = new array<ref SilverTraderMenuBuyData>;
		m_SilverBarter_PreviewPool = new map<string, EntityAI>;
		m_SilverBarter_PreviewByIndex = new map<int, EntityAI>;
		m_SilverBarter_FilterData = new array<string>;
		m_SilverBarter_DisplayNameCache = new map<string, string>;
		m_SilverBarter_PendingBuyClassnames = new array<string>;
		m_SilverBarter_PendingBuyQuantities = new array<float>;
		m_SilverBarter_BuySelectedQuantities = new map<string, float>;

		if (!s_SilverBarter_FilterMemory)
		{
			s_SilverBarter_FilterMemory = new array<bool>;
		}
	}

	void InitMetadata(int traderId, SilverTrader_Info traderInfo, SilverTrader_Data traderData, bool isRotating = false)
	{
		m_SilverBarter_TraderId = traderId;
		m_SilverBarter_IsRotatingTrader = isRotating;
		m_SilverBarter_TraderInfo = traderInfo;
		m_SilverBarter_TraderData = traderData;
		m_SilverBarter_BuySelectedQuantities.Clear();
		m_SilverBarter_Dirty = true;
	}

	void UpdateMetadata(SilverTrader_Data traderData)
	{
		int oldRevision = -1;
		if (m_SilverBarter_TraderData)
			oldRevision = m_SilverBarter_TraderData.m_rotationRevision;

		m_SilverBarter_TraderData = traderData;

		int newRevision = -1;
		if (traderData)
			newRevision = traderData.m_rotationRevision;

		// Bei Rotation (Revision geaendert) alle Previews verwerfen und neu aufbauen lassen
		if (newRevision != oldRevision)
			DiscardAllPreviews();

		NormalizeBuySelection();
		m_SilverBarter_Dirty = true;
	}

	// Entfernt tote Auswahl-Eintraege und klemmt verbleibende Mengen auf aktuellen Bestand/Max
	private void NormalizeBuySelection()
	{
		PluginSilverTrader pluginTrader = PluginSilverTrader.Cast(GetPlugin(PluginSilverTrader));
		if (!pluginTrader)
		{
			m_SilverBarter_BuySelectedQuantities.Clear();
			m_SilverBarter_Dirty = true;
			return;
		}

		for (int i = m_SilverBarter_BuySelectedQuantities.Count() - 1; i >= 0; i--)
		{
			string classname = m_SilverBarter_BuySelectedQuantities.GetKey(i);
			float selectedQuantity = m_SilverBarter_BuySelectedQuantities.GetElement(i);

			if (!m_SilverBarter_TraderData || !m_SilverBarter_TraderData.m_items || !m_SilverBarter_TraderData.m_items.Contains(classname))
			{
				m_SilverBarter_BuySelectedQuantities.Remove(classname);
				continue;
			}

			float stock = m_SilverBarter_TraderData.m_items.Get(classname);
			float maxBuy = pluginTrader.CalculateBuyMaxQuantity(m_SilverBarter_TraderInfo, classname);
			float normalizedQuantity = Math.Min(selectedQuantity, Math.Min(stock, maxBuy));

			if (normalizedQuantity <= 0)
				m_SilverBarter_BuySelectedQuantities.Remove(classname);
			else
				m_SilverBarter_BuySelectedQuantities.Set(classname, normalizedQuantity);
		}
	}

	// Leert die Kauf-Auswahl nach einem abgeschlossenen Trade (nicht bei reinem Stock-Sync)
	void ClearBuySelection()
	{
		if (m_SilverBarter_BuySelectedQuantities)
			m_SilverBarter_BuySelectedQuantities.Clear();
	}

	// Ausgeloest durch SILVERRPC_DELIVERY_COMPLETE, sobald die serverseitige Zustellung abgeschlossen ist.
	// Kein geratener Zustell-Timer mehr: die 200ms sind nur ein kleiner Client-Sync-Puffer, damit die
	// per Entity-Replikation ankommende Inventaraenderung vor dem Rebuild sicher verarbeitet ist (RPC und
	// Replikation laufen ueber getrennte Kanaele ohne garantierte Reihenfolge).
	void ScheduleSellRefresh()
	{
		m_SilverBarter_PendingSellRefreshTimer = 0.2;
	}

	void CleanupBuyUI()
	{
		if (m_SilverBarter_PendingBuyClassnames) m_SilverBarter_PendingBuyClassnames.Clear();
		if (m_SilverBarter_PendingBuyQuantities) m_SilverBarter_PendingBuyQuantities.Clear();
		m_SilverBarter_BuildIndex = 0;

		// Aktive Preview-Entities in Pool verschieben statt löschen
		if (m_SilverBarter_PreviewByIndex)
		{
			foreach (int idx, EntityAI entity : m_SilverBarter_PreviewByIndex)
			{
				if (!entity)
					continue;
				string key = GetPreviewKey(entity.GetType());
				if (m_SilverBarter_PreviewPool && m_SilverBarter_PreviewPool.Count() < PREVIEW_POOL_CAP && !m_SilverBarter_PreviewPool.Contains(key))
					m_SilverBarter_PreviewPool.Insert(key, entity);
				else
					g_Game.ObjectDelete(entity);
			}
			m_SilverBarter_PreviewByIndex.Clear();
		}

		if (m_SilverBarter_BuyWidgetsCache)
		{
			foreach (Widget w2 : m_SilverBarter_BuyWidgetsCache)
			{
				w2.Unlink();
			}
			m_SilverBarter_BuyWidgetsCache.Clear();
		}

		if (m_SilverBarter_BuyData)
		{
			m_SilverBarter_BuyData.Clear();
		}
	}

	void CleanupSellUI()
	{
		if (m_SilverBarter_SellWidgetsCache)
		{
			foreach (Widget w1 : m_SilverBarter_SellWidgetsCache)
			{
				w1.Unlink();
			}
			m_SilverBarter_SellWidgetsCache.Clear();
		}
	}

	void CleanupUI()
	{
		CleanupBuyUI();
		CleanupSellUI();
	}

	void InitInventorySell()
	{
		PluginSilverTrader pluginTrader = PluginSilverTrader.Cast(GetPlugin(PluginSilverTrader));
		if (!pluginTrader)
			return;

		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!player || !player.IsAlive())
			return;

		int nextItemIndex = -1;
		ItemBase item = ItemBase.Cast(player.GetItemInHands());
		if (item && MatchesSellSearch(item))
		{
			nextItemIndex = InitItemSell(nextItemIndex + 1, 0, item, pluginTrader);
		}

		for (int i = 0; i < player.GetInventory().GetAttachmentSlotsCount(); ++i)
		{
			item = ItemBase.Cast(player.GetInventory().FindAttachment(player.GetInventory().GetAttachmentSlotId(i)));
			if (item && MatchesSellSearch(item))
			{
				nextItemIndex = InitItemSell(nextItemIndex + 1, 0, item, pluginTrader);
			}
		}
	}

	// Suchfilter gilt nur fuer oberste Ebene (angezogene Items/Item-in-Hand) - passt ein Item nicht,
	// verschwindet es inkl. seines gesamten Unterinventars (Attachments/Cargo), keine rekursive Tiefensuche
	private bool MatchesSellSearch(ItemBase item)
	{
		if (m_SilverBarter_SellSearchText == "")
			return true;

		string dn = item.GetDisplayName();
		dn.ToLower();
		return dn.IndexOf(m_SilverBarter_SellSearchText) != -1;
	}

	int InitItemSell(int index, int depth, ItemBase item, PluginSilverTrader pluginTrader)
	{
		int screenWidth;
		int screenHeight;
		GetScreenSize(screenWidth, screenHeight);

		Widget itemSell;
		if (DEBUG_USE_TEST_LAYOUT)
		{
			itemSell = g_Game.GetWorkspace().CreateWidgets("SilverBarter/layout/test/TraderMenuItemSell.layout");
		}
		else if (screenHeight > 1440)
		{
			itemSell = g_Game.GetWorkspace().CreateWidgets("SilverBarter/layout/2160p/TraderMenuItemSell.layout");
		}
		else
		{
			itemSell = g_Game.GetWorkspace().CreateWidgets("SilverBarter/layout/TraderMenuItemSell.layout");
		}

		m_SilverBarter_SellItemsPanel.AddChild(itemSell);

		float w, h;
		float contentWidth = m_SilverBarter_SellItemsPanel.GetContentWidth() - (SELL_ITEM_DEPTH_OFFSET * depth);
		itemSell.GetSize(w, h);
		itemSell.SetPos(SELL_ITEM_DEPTH_OFFSET * depth, (h + SELL_ITEM_HEIGHT_OFFSET) * index);
		itemSell.SetSize(contentWidth, h);

		ButtonWidget actionButton = ButtonWidget.Cast(itemSell.FindAnyWidget("ItemActionButton"));
		actionButton.SetUserData(item);
		actionButton.SetUserID(1001);
		actionButton.GetParent().SetUserID(depth);

		ItemPreviewWidget previewWidget = ItemPreviewWidget.Cast(itemSell.FindAnyWidget("ItemPreviewWidget"));
		previewWidget.SetItem(item);
		previewWidget.SetView(item.GetViewIndex());
		previewWidget.SetModelPosition(Vector(0, 0, 1));

		WidgetSetWidth(itemSell, "ItemNameWidget", contentWidth - 220);
		WidgetTrySetText(itemSell, "ItemNameWidget", item.GetDisplayName());

		// Preis immer verstecken - "Nicht zum Verkauf" wird im ItemQuantityWidget angezeigt
		WidgetTrySetText(itemSell, "ItemPriceWidget", " ");

		UpdateItemInfoDamage(itemSell, item);
		UpdateItemInfoQuantity(itemSell, item);
		m_SilverBarter_SellWidgetsCache.Insert(itemSell);

		if (item.GetInventory() && depth < 8)
		{
			int i;
			for (i = 0; i < item.GetInventory().GetAttachmentSlotsCount(); ++i)
			{
				ItemBase attachment = ItemBase.Cast(item.GetInventory().FindAttachment(item.GetInventory().GetAttachmentSlotId(i)));
				if (attachment)
				{
					index = InitItemSell(index + 1, depth + 1, attachment, pluginTrader);
				}
			}

			if (item.GetInventory().GetCargo())
			{
				for (i = 0; i < item.GetInventory().GetCargo().GetItemCount(); ++i)
				{
					ItemBase cargo = ItemBase.Cast(item.GetInventory().GetCargo().GetItem(i));
					if (cargo)
					{
						index = InitItemSell(index + 1, depth + 1, cargo, pluginTrader);
					}
				}
			}
		}

		return index;
	}

	void InitInventoryBuy()
	{
		CleanupBuyUI();
		m_SilverBarter_BuyItemsPanel.VScrollToPos01(0);
		m_SilverBarter_LastScrollPos01 = -1;
		m_SilverBarter_BuyRowHeight = 0;
		m_SilverBarter_BuyPanelHeight = 0;

		int sw;
		GetScreenSize(sw, m_SilverBarter_CachedScreenHeight);

		PluginSilverTrader pluginTrader = PluginSilverTrader.Cast(GetPlugin(PluginSilverTrader));
		if (!pluginTrader || !m_SilverBarter_TraderData || !m_SilverBarter_TraderData.m_items)
			return;

		array<string> sortKeys = new array<string>;
		map<string, string> keyToClass = new map<string, string>;

		foreach (string cn, float qty : m_SilverBarter_TraderData.m_items)
		{
			if (!pluginTrader.FilterByCategories(m_SilverBarter_FilterData, s_SilverBarter_FilterMemory, cn))
				continue;

			string dn = GetItemDisplayName(cn);
			dn.ToLower();

			if (m_SilverBarter_BuySearchText != "" && dn.IndexOf(m_SilverBarter_BuySearchText) == -1)
				continue;

			string key = dn + "|" + cn;
			sortKeys.Insert(key);
			keyToClass.Insert(key, cn);
		}

		sortKeys.Sort(false);

		foreach (string key2 : sortKeys)
		{
			string classname = keyToClass.Get(key2);
			m_SilverBarter_PendingBuyClassnames.Insert(classname);
			m_SilverBarter_PendingBuyQuantities.Insert(m_SilverBarter_TraderData.m_items.Get(classname));
		}
	}

	private void StepBuildBuyList()
	{
		if (!m_SilverBarter_PendingBuyClassnames || m_SilverBarter_BuildIndex >= m_SilverBarter_PendingBuyClassnames.Count())
			return;

		PluginSilverTrader pluginTrader = PluginSilverTrader.Cast(GetPlugin(PluginSilverTrader));
		if (!pluginTrader)
			return;

		int limit = m_SilverBarter_BuildIndex + BUILD_BATCH_SIZE;
		if (limit > m_SilverBarter_PendingBuyClassnames.Count())
			limit = m_SilverBarter_PendingBuyClassnames.Count();

		for (int i = m_SilverBarter_BuildIndex; i < limit; i++)
		{
			InitItemBuy(i, m_SilverBarter_PendingBuyClassnames.Get(i), m_SilverBarter_PendingBuyQuantities.Get(i), pluginTrader);
		}
		m_SilverBarter_BuildIndex = limit;

		// UpdateLazyPreviews springt sonst raus wenn Scrollpos gleich bleibt
		m_SilverBarter_LastScrollPos01 = -1;
	}

	private string GetItemDisplayName(string classname)
	{
		if (m_SilverBarter_DisplayNameCache.Contains(classname))
			return m_SilverBarter_DisplayNameCache.Get(classname);

		string dn = classname;
		if (g_Game.ConfigIsExisting(CFG_VEHICLESPATH + " " + classname + " displayName"))
			dn = g_Game.ConfigGetTextOut(CFG_VEHICLESPATH + " " + classname + " displayName");
		else if (g_Game.ConfigIsExisting(CFG_MAGAZINESPATH + " " + classname + " displayName"))
			dn = g_Game.ConfigGetTextOut(CFG_MAGAZINESPATH + " " + classname + " displayName");
		else if (g_Game.ConfigIsExisting(CFG_WEAPONSPATH + " " + classname + " displayName"))
			dn = g_Game.ConfigGetTextOut(CFG_WEAPONSPATH + " " + classname + " displayName");

		m_SilverBarter_DisplayNameCache.Insert(classname, dn);
		return dn;
	}

	// Kein ItemBase-Parameter mehr - Preview wird lazy in UpdateLazyPreviews() gesetzt
	int InitItemBuy(int index, string classname, float quantity, PluginSilverTrader pluginTrader)
	{
		Widget itemBuy;
		if (DEBUG_USE_TEST_LAYOUT)
		{
			itemBuy = g_Game.GetWorkspace().CreateWidgets("SilverBarter/layout/test/TraderMenuItemBuy.layout");
		}
		else if (m_SilverBarter_CachedScreenHeight > 1440)
		{
			itemBuy = g_Game.GetWorkspace().CreateWidgets("SilverBarter/layout/2160p/TraderMenuItemBuy.layout");
		}
		else
		{
			itemBuy = g_Game.GetWorkspace().CreateWidgets("SilverBarter/layout/TraderMenuItemBuy.layout");
		}

		m_SilverBarter_BuyItemsPanel.AddChild(itemBuy);

		float w, h;
		float contentWidth = m_SilverBarter_BuyItemsPanel.GetContentWidth();
		itemBuy.GetSize(w, h);

		// Zeilenhöhe beim ersten Item merken
		if (m_SilverBarter_BuyRowHeight <= 0)
			m_SilverBarter_BuyRowHeight = h + SELL_ITEM_HEIGHT_OFFSET;

		itemBuy.SetPos(0, m_SilverBarter_BuyRowHeight * index);
		itemBuy.SetSize(contentWidth, h);
		itemBuy.SetUserID(index);

		SilverTraderMenuBuyData actionBtnParam = new SilverTraderMenuBuyData;
		actionBtnParam.m_Classname = classname;
		actionBtnParam.m_TotalQuantity = quantity;
		actionBtnParam.m_MaxBuyQuantity = Math.Min(pluginTrader.CalculateBuyMaxQuantity(m_SilverBarter_TraderInfo, classname), quantity);

		ButtonWidget actionButton = ButtonWidget.Cast(itemBuy.FindAnyWidget("ItemActionButton"));

		// Vorherige Auswahl wiederherstellen (uebersteht Filter-Toggle/Rebuild); Map bleibt einzige Quelle der Wahrheit
		float initialQty;
		if (m_SilverBarter_BuySelectedQuantities.Contains(classname))
		{
			initialQty = Math.Min(m_SilverBarter_BuySelectedQuantities.Get(classname), actionBtnParam.m_MaxBuyQuantity);
			m_SilverBarter_BuySelectedQuantities.Set(classname, initialQty);
			actionButton.SetUserID(2002);
			Widget actionButtonBack = actionButton.GetParent();
			if (actionButtonBack)
				actionButtonBack.SetColor(ARGB(200, 16, 87, 20));
		}
		else
		{
			initialQty = Math.Min(1, actionBtnParam.m_MaxBuyQuantity);
			actionButton.SetUserID(2001);
		}

		// PreviewWidget bleibt leer - wird lazy befüllt
		string displayName = GetItemDisplayName(classname);

		WidgetSetWidth(itemBuy, "ItemNameWidget", contentWidth - 220);
		WidgetTrySetText(itemBuy, "ItemNameWidget", displayName);

		// Preis verstecken - Balken-System zeigt Tauschwert
		WidgetTrySetText(itemBuy, "ItemPriceWidget", " ");

		UpdateItemInfoQuantity(itemBuy, pluginTrader, classname, quantity);
		UpdateItemInfoSelectedQuantity(itemBuy, classname, initialQty, actionBtnParam.m_MaxBuyQuantity);

		ButtonWidget minusButton = ButtonWidget.Cast(itemBuy.FindAnyWidget("MinusActionBtn"));
		minusButton.SetUserID(3001);

		ButtonWidget plusButton = ButtonWidget.Cast(itemBuy.FindAnyWidget("PlusActionBtn"));
		plusButton.SetUserID(3002);

		m_SilverBarter_BuyWidgetsCache.Insert(itemBuy);
		m_SilverBarter_BuyData.Insert(actionBtnParam);
		return index;
	}

	// Spawnt/despawnt Preview-Entities je nach Sichtbarkeit, max 4 Spawns pro Frame
	// Pool-Schluessel inkl. Rotation-Revision: verhindert, dass nach einer Rotation eine alt bestueckte Preview
	// desselben Classname wiederverwendet wird.
	private string GetPreviewKey(string classname)
	{
		int rev = 0;
		if (m_SilverBarter_TraderData)
			rev = m_SilverBarter_TraderData.m_rotationRevision;
		return classname + ":" + rev.ToString();
	}

	// Haengt die gesyncten Preview-Attachments lokal an eine frisch erzeugte Preview-Waffe an (rein visuell).
	// Iterativ ueber die flache Liste; das created-Array bleibt index-gleich, damit m_ParentIndex das richtige Parent trifft.
	private void ApplyPreviewAttachments(EntityAI weapon, string classname)
	{
		if (!weapon || !m_SilverBarter_TraderData || !m_SilverBarter_TraderData.m_previewAttachments)
			return;

		array<ref SilverPreviewAttachment> list;
		if (!m_SilverBarter_TraderData.m_previewAttachments.Find(classname, list) || !list)
			return;

		array<EntityAI> created = new array<EntityAI>;
		int count = list.Count();
		for (int i = 0; i < count; i++)
		{
			SilverPreviewAttachment pa = list.Get(i);
			if (!pa || pa.m_Classname == "")
			{
				created.Insert(null);
				continue;
			}

			EntityAI parent = weapon;
			if (pa.m_ParentIndex >= 0 && pa.m_ParentIndex < created.Count())
				parent = created.Get(pa.m_ParentIndex);

			if (!parent)
			{
				created.Insert(null);
				continue;
			}

			EntityAI att = null;
			if (pa.m_Slot != "")
			{
				int slotId = InventorySlots.GetSlotIdFromString(pa.m_Slot);
				if (slotId != InventorySlots.INVALID)
					att = parent.GetInventory().CreateAttachmentEx(pa.m_Classname, slotId);
			}
			else
			{
				att = parent.GetInventory().CreateAttachment(pa.m_Classname);
			}
			created.Insert(att);
		}

		// Lokale Preview-Waffen aktualisieren ihre Magazin-Selektion nicht automatisch.
		Weapon_Base previewWeapon = Weapon_Base.Cast(weapon);
		if (previewWeapon)
			previewWeapon.ForceSyncSelectionState();
	}

	// Verwirft alle aktiven und gepoolten Preview-Entities (bei Rotation), damit keine veraltete Bestueckung bleibt.
	private void DiscardAllPreviews()
	{
		if (m_SilverBarter_PreviewByIndex && m_SilverBarter_BuyWidgetsCache)
		{
			int widgetCount = m_SilverBarter_BuyWidgetsCache.Count();
			for (int bi = 0; bi < widgetCount; bi++)
			{
				if (!m_SilverBarter_PreviewByIndex.Contains(bi))
					continue;
				Widget w = m_SilverBarter_BuyWidgetsCache.Get(bi);
				if (w)
				{
					ItemPreviewWidget pv = ItemPreviewWidget.Cast(w.FindAnyWidget("ItemPreviewWidget"));
					if (pv)
						pv.SetItem(null);
				}
			}

			int activeCount = m_SilverBarter_PreviewByIndex.Count();
			for (int ai = 0; ai < activeCount; ai++)
			{
				EntityAI e = m_SilverBarter_PreviewByIndex.GetElement(ai);
				if (e)
					g_Game.ObjectDelete(e);
			}
			m_SilverBarter_PreviewByIndex.Clear();
		}

		if (m_SilverBarter_PreviewPool)
		{
			int poolCount = m_SilverBarter_PreviewPool.Count();
			for (int pi = 0; pi < poolCount; pi++)
			{
				EntityAI pe = m_SilverBarter_PreviewPool.GetElement(pi);
				if (pe)
					g_Game.ObjectDelete(pe);
			}
			m_SilverBarter_PreviewPool.Clear();
		}

		// Erzwingt einen erneuten Lazy-Preview-Aufbau (sonst blockiert der Scroll-Gleichstand-Check)
		m_SilverBarter_LastScrollPos01 = -1;
	}

	private void UpdateLazyPreviews()
	{
		if (!m_SilverBarter_BuyWidgetsCache || m_SilverBarter_BuyWidgetsCache.Count() == 0 || m_SilverBarter_BuyRowHeight <= 0)
			return;

		// Panel-Höhe lazy ermitteln (erst nach erstem Layout-Pass verfügbar)
		if (m_SilverBarter_BuyPanelHeight <= 0)
		{
			float pw, ph;
			m_SilverBarter_BuyItemsPanel.GetSize(pw, ph);
			m_SilverBarter_BuyPanelHeight = ph;
			if (m_SilverBarter_BuyPanelHeight <= 0)
				return;
		}

		float scrollPos01 = m_SilverBarter_BuyItemsPanel.GetVScrollPos01();
		if (Math.AbsFloat(scrollPos01 - m_SilverBarter_LastScrollPos01) < 0.001)
			return;

		float contentH = m_SilverBarter_BuyItemsPanel.GetContentHeight();
		float scrollPx = scrollPos01 * Math.Max(0, contentH - m_SilverBarter_BuyPanelHeight);
		float visibleTop = scrollPx - m_SilverBarter_BuyRowHeight;
		float visibleBottom = scrollPx + m_SilverBarter_BuyPanelHeight + m_SilverBarter_BuyRowHeight;

		int count = m_SilverBarter_BuyWidgetsCache.Count();

		// Pass 1: Despawn - alle außerhalb des Sichtbereichs → Pool
		// Iteration über Widget-Indizes (nicht über die Map) → Remove() sicher
		for (int di = 0; di < count; di++)
		{
			if (!m_SilverBarter_PreviewByIndex.Contains(di))
				continue;
			float rowTop = m_SilverBarter_BuyRowHeight * di;
			if (rowTop < visibleBottom && (rowTop + m_SilverBarter_BuyRowHeight) > visibleTop)
				continue; // noch sichtbar

			// Preview entkoppeln
			Widget dw = m_SilverBarter_BuyWidgetsCache.Get(di);
			if (dw)
			{
				ItemPreviewWidget pv = ItemPreviewWidget.Cast(dw.FindAnyWidget("ItemPreviewWidget"));
				if (pv)
					pv.SetItem(null);
			}

			EntityAI de = m_SilverBarter_PreviewByIndex.Get(di);
			if (de)
			{
				string dkey = GetPreviewKey(de.GetType());
				if (m_SilverBarter_PreviewPool.Count() < PREVIEW_POOL_CAP && !m_SilverBarter_PreviewPool.Contains(dkey))
					m_SilverBarter_PreviewPool.Insert(dkey, de);
				else
					g_Game.ObjectDelete(de);
			}
			m_SilverBarter_PreviewByIndex.Remove(di);
		}

		// Pass 2: Spawn - nur im sichtbaren Bereich, max 4 pro Frame
		// Despawns sind fertig → break nach Limit ist korrekt
		int startIndex = (int)Math.Floor(visibleTop / m_SilverBarter_BuyRowHeight);
		int endIndex = (int)Math.Ceil(visibleBottom / m_SilverBarter_BuyRowHeight);
		startIndex = Math.Clamp(startIndex, 0, count - 1);
		endIndex = Math.Clamp(endIndex, 0, count - 1);

		int spawnsThisFrame = 0;
		for (int i = startIndex; i <= endIndex; i++)
		{
			if (m_SilverBarter_PreviewByIndex.Contains(i))
				continue;

			// Absicherung gegen Layout-Rounding
			float rowTop2 = m_SilverBarter_BuyRowHeight * i;
			if (!(rowTop2 < visibleBottom && (rowTop2 + m_SilverBarter_BuyRowHeight) > visibleTop))
				continue;

			if (spawnsThisFrame >= 4)
				break;

			SilverTraderMenuBuyData data = m_SilverBarter_BuyData.Get(i);
			if (!data)
				continue;

			EntityAI entity = null;
			string previewKey = GetPreviewKey(data.m_Classname);
			if (m_SilverBarter_PreviewPool.Contains(previewKey))
			{
				entity = m_SilverBarter_PreviewPool.Get(previewKey);
				m_SilverBarter_PreviewPool.Remove(previewKey);
			}
			else
			{
				Object obj = g_Game.CreateObject(data.m_Classname, "0 0 0", true, false, false);
				entity = EntityAI.Cast(obj);
				// Nur frisch erzeugte Previews bestuecken; aus dem Pool geholte tragen ihre Attachments bereits.
				if (entity)
					ApplyPreviewAttachments(entity, data.m_Classname);
			}

			if (!entity)
				continue;

			m_SilverBarter_PreviewByIndex.Insert(i, entity);
			ItemBase item = ItemBase.Cast(entity);
			ItemPreviewWidget preview = ItemPreviewWidget.Cast(m_SilverBarter_BuyWidgetsCache.Get(i).FindAnyWidget("ItemPreviewWidget"));
			if (preview && item)
			{
				preview.SetItem(item);
				preview.SetView(item.GetViewIndex());
				preview.SetModelPosition(Vector(0, 0, 1));
			}
			spawnsThisFrame++;
		}

		// Scroll-State nur speichern wenn keine Spawns mehr ausstehen
		if (spawnsThisFrame < 4)
			m_SilverBarter_LastScrollPos01 = scrollPos01;
	}

	string FormatBuyQuantityStr(float quantity)
	{
		int quantityInt = (int)Math.Round(quantity * 10.0);
		string quantityStr = quantityInt.ToString();

		if (quantityInt % 10 != 0)
		{
			if (quantityStr.Length() == 1)
			{
				quantityStr = "0." + quantityStr;
			}
			else
			{
				quantityStr = quantityStr.Substring(0, quantityStr.Length() - 1) + "." + quantityStr.Substring(quantityStr.Length() - 1, 1);
			}
		}
		else
		{
			if (quantityStr.Length() > 1)
			{
				quantityStr = quantityStr.Substring(0, quantityStr.Length() - 1);
			}
		}

		return quantityStr;
	}

	void UpdateCurrentPriceProgress()
	{
		PluginSilverTrader pluginTrader = PluginSilverTrader.Cast(GetPlugin(PluginSilverTrader));
		if (!pluginTrader)
			return;

		int blockedItemsCounter = 0;
		float value = 0;
		array<ItemBase> sellResult = new array<ItemBase>;
		map<string, float> sellCounter = new map<string, float>;

		GetSelectedSellItems(sellResult);
		foreach (ItemBase sellItem : sellResult)
		{
			if (!sellItem)
				continue;

			string classname = sellItem.GetType();
			if (sellCounter.Contains(classname))
			{
				sellCounter.Set(classname, sellCounter.Get(classname) + pluginTrader.CalculateItemQuantity01(sellItem));
			}
			else
			{
				sellCounter.Insert(classname, pluginTrader.CalculateItemQuantity01(sellItem));
			}

			if (pluginTrader.CanSellItem(m_SilverBarter_TraderInfo, sellItem))
			{
				value = value + pluginTrader.CalculateSellPrice(m_SilverBarter_TraderInfo, m_SilverBarter_TraderData, sellItem);
			}
			else
			{
				blockedItemsCounter++;
			}
		}
		sellResult = null;

		map<string, float> buyResult = new map<string, float>;
		GetSelectedBuyItems(buyResult);
		foreach (string buyClassname, float buyQuantity : buyResult)
		{
			if (pluginTrader.CanBuyItem(m_SilverBarter_TraderInfo, buyClassname))
			{
				value = value - pluginTrader.CalculateBuyPriceWithAttachments(m_SilverBarter_TraderInfo, m_SilverBarter_TraderData, buyClassname, buyQuantity);
			}
			else
			{
				blockedItemsCounter++;
			}
		}
		buyResult = null;

		value = value / PROGRESS_BAR_PRICE_DIVIDER;

		if (value > 0)
		{
			m_SilverBarter_ProgressPositive.SetCurrent(Math.Min(100, value));
			m_SilverBarter_ProgressNegative.SetCurrent(0);
			m_SilverBarter_BarterButton.Enable(true);
			m_SilverBarter_BarterButton.SetColor(TRADE_BUTTON_COLOR_ENABLED);
		}
		else if (value < 0)
		{
			m_SilverBarter_ProgressPositive.SetCurrent(0);
			m_SilverBarter_ProgressNegative.SetCurrent(Math.Min(100, value * -1));
			m_SilverBarter_BarterButton.Enable(false);
			m_SilverBarter_BarterButton.SetColor(TRADE_BUTTON_COLOR_DISABLED);
		}
		else
		{
			m_SilverBarter_ProgressPositive.SetCurrent(0);
			m_SilverBarter_ProgressNegative.SetCurrent(0);
			m_SilverBarter_BarterButton.Enable(true);
			m_SilverBarter_BarterButton.SetColor(TRADE_BUTTON_COLOR_ENABLED);
		}

		if (blockedItemsCounter > 0)
		{
			m_SilverBarter_TradeButtonInfo.SetText("#silver_trader_block_baditems");
			m_SilverBarter_BlockBarter = true;
		}
		else if (!m_SilverBarter_IsRotatingTrader && pluginTrader.HasOversizedSellItems(m_SilverBarter_TraderInfo, m_SilverBarter_TraderData, sellCounter))
		{
			m_SilverBarter_TradeButtonInfo.SetText("#silver_trader_block_toomany");
			m_SilverBarter_BlockBarter = true;
		}
		else
		{
			m_SilverBarter_TradeButtonInfo.SetText("");
			m_SilverBarter_BlockBarter = false;
		}

		// Button-State final: blockBarter hat Vorrang
		if (m_SilverBarter_BlockBarter)
		{
			m_SilverBarter_BarterButton.Enable(false);
			m_SilverBarter_BarterButton.SetColor(TRADE_BUTTON_COLOR_DISABLED);
		}

		sellCounter = null;
		m_SilverBarter_CurrentBarterProgress = value;
	}

	void GetSelectedSellItems(array<ItemBase> result)
	{
		foreach (Widget w : m_SilverBarter_SellWidgetsCache)
		{
			Widget btn = w.FindAnyWidget("ItemActionButton");
			if (btn.GetUserID() == 1002)
			{
				ItemBase item;
				btn.GetUserData(item);
				if (item)
				{
					result.Insert(item);
				}
			}
		}
	}

	void GetSelectedBuyItems(map<string, float> result)
	{
		foreach (string classname, float selectedQuantity : m_SilverBarter_BuySelectedQuantities)
		{
			result.Insert(classname, selectedQuantity);
		}
	}

	void InitializeFilter(Widget root, string name)
	{
		int id = m_SilverBarter_FilterData.Insert(name);
		ButtonWidget btn = ButtonWidget.Cast(root.FindAnyWidget("FilterActionBtn" + id));
		btn.SetUserID(5000 + id);

		TextWidget btnText = TextWidget.Cast(btn.GetChildren());
		btnText.SetText("#silver_trader_filter_" + name);

		if (s_SilverBarter_FilterMemory.Count() <= id)
		{
			s_SilverBarter_FilterMemory.Insert(true);
		}

		SelectFilterItem(btn, s_SilverBarter_FilterMemory.Get(id));
	}

	override Widget Init()
	{
		int screenWidth;
		int screenHeight;
		GetScreenSize(screenWidth, screenHeight);

		if (DEBUG_USE_TEST_LAYOUT)
		{
			layoutRoot = g_Game.GetWorkspace().CreateWidgets("SilverBarter/layout/test/TraderMenu.layout");
		}
		else if (screenHeight > 1440)
		{
			layoutRoot = g_Game.GetWorkspace().CreateWidgets("SilverBarter/layout/2160p/TraderMenu.layout");
		}
		else
		{
			layoutRoot = g_Game.GetWorkspace().CreateWidgets("SilverBarter/layout/TraderMenu.layout");
		}

		m_SilverBarter_SellItemsPanel = ScrollWidget.Cast(layoutRoot.FindAnyWidget("SellItemsPanel"));
		m_SilverBarter_BuyItemsPanel = ScrollWidget.Cast(layoutRoot.FindAnyWidget("BuyItemsPanel"));
		m_SilverBarter_ProgressPositive = SimpleProgressBarWidget.Cast(layoutRoot.FindAnyWidget("ProgressPositive"));
		m_SilverBarter_ProgressNegative = SimpleProgressBarWidget.Cast(layoutRoot.FindAnyWidget("ProgressNegative"));
		m_SilverBarter_BarterButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("TradeButton"));
		m_SilverBarter_TradeButtonInfo = MultilineTextWidget.Cast(layoutRoot.FindAnyWidget("TradeButtonInfo"));

		// Nur im Test-Layout vorhanden - bleibt null im alten Layout, Aufrufer pruefen darauf
		m_SilverBarter_BuySearchBox = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("BuySearchBox"));
		m_SilverBarter_SellSearchBox = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("SellSearchBox"));

		m_SilverBarter_FilterData.Clear();
		InitializeFilter(layoutRoot, "weapons");
		InitializeFilter(layoutRoot, "magazines");
		InitializeFilter(layoutRoot, "attachments");
		InitializeFilter(layoutRoot, "ammo");
		InitializeFilter(layoutRoot, "tools");
		InitializeFilter(layoutRoot, "food");
		InitializeFilter(layoutRoot, "clothing");
		InitializeFilter(layoutRoot, "medical");
		InitializeFilter(layoutRoot, "electronic");
		InitializeFilter(layoutRoot, "base_building");
		InitializeFilter(layoutRoot, "vehicle_parts");
		InitializeFilter(layoutRoot, "other");

		m_SilverBarter_Active = true;
		return layoutRoot;
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);

		if (m_SilverBarter_Dirty)
		{
			CleanupUI();
			InitInventorySell();
			InitInventoryBuy();
			UpdateCurrentPriceProgress();
			m_SilverBarter_Dirty = false;
		}

		if (m_SilverBarter_PendingSellRefreshTimer > 0)
		{
			m_SilverBarter_PendingSellRefreshTimer = m_SilverBarter_PendingSellRefreshTimer - timeslice;

			if (m_SilverBarter_PendingSellRefreshTimer <= 0)
			{
				m_SilverBarter_PendingSellRefreshTimer = -1;

				if (m_SilverBarter_Active)
					m_SilverBarter_Dirty = true;
			}
		}

		if (m_SilverBarter_PendingSellSearchTimer > 0)
		{
			m_SilverBarter_PendingSellSearchTimer = m_SilverBarter_PendingSellSearchTimer - timeslice;

			if (m_SilverBarter_PendingSellSearchTimer <= 0)
			{
				m_SilverBarter_PendingSellSearchTimer = -1;

				if (m_SilverBarter_Active)
				{
					CleanupSellUI();
					InitInventorySell();
					UpdateCurrentPriceProgress();
				}
			}
		}

		if (m_SilverBarter_PendingBuySearchTimer > 0)
		{
			m_SilverBarter_PendingBuySearchTimer = m_SilverBarter_PendingBuySearchTimer - timeslice;

			if (m_SilverBarter_PendingBuySearchTimer <= 0)
			{
				m_SilverBarter_PendingBuySearchTimer = -1;

				if (m_SilverBarter_Active)
				{
					InitInventoryBuy();
					UpdateCurrentPriceProgress();
				}
			}
		}

		StepBuildBuyList();
		UpdateLazyPreviews();

		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!player || !player.IsAlive() || player.IsUnconscious() || player.IsRestrained())
		{
			m_SilverBarter_Active = false;
		}
		else if (m_SilverBarter_TraderInfo && vector.Distance(m_SilverBarter_TraderInfo.m_position, player.GetPosition()) > 5)
		{
			m_SilverBarter_Active = false;
		}

		if (!m_SilverBarter_Active)
		{
			g_Game.GetUIManager().Back();
		}
		else
		{
			// Focus wiederherstellen nach Alt+Tab
			g_Game.GetInput().ChangeGameFocus(1);

			// SetFocus(layoutRoot) nicht jeden Frame erzwingen - sonst reisst es den Fokus
			// aus einem aktiven Suchfeld (EditBoxWidget) heraus und Tippen ist unmoeglich
			if (!GetFocus())
				SetFocus(layoutRoot);
		}
	}

	override void OnShow()
	{
		super.OnShow();
		g_Game.GetInput().ChangeGameFocus(1);
		SetFocus(layoutRoot);

		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (player)
		{
			player.GetInputController().SetDisabled(true);
			player.GetActionManager().EnableActions(false);
		}
	}

	override void OnHide()
	{
		super.OnHide();
		g_Game.GetInput().ResetGameFocus();

		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (player)
		{
			player.GetInputController().SetDisabled(false);
			player.GetActionManager().EnableActions(true);
		}

		// Close-RPC senden
		PlayerBase closePlayer = PlayerBase.Cast(g_Game.GetPlayer());
		if (closePlayer)
		{
			ScriptRPC closeRpc = new ScriptRPC();
			closeRpc.Write(SilverRPC.SILVERRPC_CLOSE_TRADER_MENU);
			closeRpc.Write(m_SilverBarter_TraderId);
			closeRpc.Send(closePlayer, SilverRPCManager.CHANNEL_SILVER_BARTER, true);

			PluginSilverTrader pluginTrader = PluginSilverTrader.Cast(GetPlugin(PluginSilverTrader));
			if (pluginTrader)
				pluginTrader.DebugLog("Close RPC sent for trader " + m_SilverBarter_TraderId.ToString());
		}

		CleanupUI(); // verschiebt aktive Entities in Pool
		m_SilverBarter_PendingSellRefreshTimer = -1;

		// Widget-Referenzen freigeben
		m_SilverBarter_SellItemsPanel = null;
		m_SilverBarter_BuyItemsPanel = null;
		m_SilverBarter_ProgressPositive = null;
		m_SilverBarter_ProgressNegative = null;
		m_SilverBarter_BarterButton = null;
		m_SilverBarter_TradeButtonInfo = null;
		m_SilverBarter_BuySelectedQuantities = null;

		m_SilverBarter_BuySearchBox = null;
		m_SilverBarter_SellSearchBox = null;
		m_SilverBarter_BuySearchText = "";
		m_SilverBarter_SellSearchText = "";
		m_SilverBarter_PendingSellSearchTimer = -1;
		m_SilverBarter_PendingBuySearchTimer = -1;

		PluginSilverTrader traderPlugin = PluginSilverTrader.Cast(GetPlugin(PluginSilverTrader));
		if (traderPlugin)
			traderPlugin.ClearTraderMenuRef(this);

		// Pool-Entities jetzt final löschen
		if (m_SilverBarter_PreviewPool)
		{
			CGame game = g_Game;
			for (int pi = 0; pi < m_SilverBarter_PreviewPool.Count(); pi++)
			{
				EntityAI e = m_SilverBarter_PreviewPool.GetElement(pi);
				if (e && game)
					game.ObjectDelete(e);
			}
			m_SilverBarter_PreviewPool = null;
		}

		m_SilverBarter_PreviewByIndex = null;
		m_SilverBarter_TraderInfo = null;
		m_SilverBarter_TraderData = null;
		m_SilverBarter_BuyData = null;
		m_SilverBarter_SellWidgetsCache = null;
		m_SilverBarter_BuyWidgetsCache = null;
		m_SilverBarter_FilterData = null;
		m_SilverBarter_DisplayNameCache = null;
	}

	private void SelectSellItem(ButtonWidget btn, bool enable)
	{
		Widget back = btn.GetParent();
		if (!back)
			return;

		int depth = back.GetUserID();
		int itemId = m_SilverBarter_SellWidgetsCache.Find(back.GetParent());

		if (itemId != -1)
		{
			int index = itemId - 1;

			if (btn.GetUserID() == 1002)
			{
				while (index >= 0)
				{
					Widget prevItem = m_SilverBarter_SellWidgetsCache.Get(index);
					ButtonWidget prevBtn = ButtonWidget.Cast(prevItem.FindAnyWidget("ItemActionButton"));
					Widget prevBack = prevBtn.GetParent();

					if (prevBack.GetUserID() < depth)
					{
						if (prevBtn.GetUserID() == 1002)
						{
							prevBack.SetColor(ARGB(200, 25, 25, 25));
							prevBtn.SetUserID(1001);
						}
						break;
					}
					index--;
				}
			}

			if (enable)
			{
				back.SetColor(ARGB(200, 16, 87, 20));
				btn.SetUserID(1002);
			}
			else
			{
				back.SetColor(ARGB(200, 25, 25, 25));
				btn.SetUserID(1001);
			}

			index = itemId + 1;
			while (index < m_SilverBarter_SellWidgetsCache.Count())
			{
				Widget nextItem = m_SilverBarter_SellWidgetsCache.Get(index);
				ButtonWidget nextBtn = ButtonWidget.Cast(nextItem.FindAnyWidget("ItemActionButton"));
				Widget nextBack = nextBtn.GetParent();

				if (depth >= nextBack.GetUserID())
				{
					break;
				}

				if (enable)
				{
					nextBack.SetColor(ARGB(200, 16, 87, 20));
					nextBtn.SetUserID(1002);
				}
				else
				{
					nextBack.SetColor(ARGB(200, 25, 25, 25));
					nextBtn.SetUserID(1001);
				}
				index++;
			}
		}

		UpdateCurrentPriceProgress();
	}

	private void SelectBuyItem(ButtonWidget btn, bool enable)
	{
		Widget back = btn.GetParent();
		if (!back)
			return;

		if (enable)
		{
			back.SetColor(ARGB(200, 16, 87, 20));
			btn.SetUserID(2002);
		}
		else
		{
			back.SetColor(ARGB(200, 25, 25, 25));
			btn.SetUserID(2001);
		}

		// Persistente Auswahl nachfuehren, damit Filter-Toggles/Rebuilds sie nicht verlieren.
		// Map ist einzige Quelle der Wahrheit - bei bereits vorhandenem Eintrag (z.B. von ChangeBuyQuantity()
		// unmittelbar zuvor gesetzt) nichts ueberschreiben, nur beim erstmaligen Auswaehlen den Default eintragen.
		Widget mainWidget = back.GetParent();
		if (mainWidget)
		{
			int index = mainWidget.GetUserID();
			if (index >= 0 && index < m_SilverBarter_BuyData.Count())
			{
				SilverTraderMenuBuyData buyData = m_SilverBarter_BuyData.Get(index);
				if (buyData)
				{
					if (enable)
					{
						if (!m_SilverBarter_BuySelectedQuantities.Contains(buyData.m_Classname))
							m_SilverBarter_BuySelectedQuantities.Set(buyData.m_Classname, Math.Min(1, buyData.m_MaxBuyQuantity));
					}
					else
					{
						m_SilverBarter_BuySelectedQuantities.Remove(buyData.m_Classname);
					}
				}
			}
		}

		UpdateCurrentPriceProgress();
	}

	private void SwitchFilterItem(ButtonWidget btn)
	{
		Widget back = btn.GetParent();
		if (!back)
			return;

		int value = back.GetUserID();
		if (value == 0)
		{
			SelectFilterItem(btn, true);
		}
		else
		{
			SelectFilterItem(btn, false);
		}

		InitInventoryBuy();
		UpdateCurrentPriceProgress();
	}

	private void SelectFilterItem(ButtonWidget btn, bool enable)
	{
		s_SilverBarter_FilterMemory.Set(btn.GetUserID() - 5000, enable);

		Widget back = btn.GetParent();
		if (!back)
			return;

		if (enable)
		{
			back.SetColor(ARGB(200, 16, 87, 20));
			back.SetUserID(1);
		}
		else
		{
			back.SetColor(ARGB(200, 25, 25, 25));
			back.SetUserID(0);
		}
	}

	private void ChangeBuyQuantity(ButtonWidget btn, float value)
	{
		PluginSilverTrader pluginTrader = PluginSilverTrader.Cast(GetPlugin(PluginSilverTrader));
		if (!pluginTrader)
			return;

		Widget mainWidget = btn.GetParent().GetParent().GetParent().GetParent();
		int id = mainWidget.GetUserID();
		if (id < 0 || id >= m_SilverBarter_BuyData.Count())
			return;

		ButtonWidget mainButton = ButtonWidget.Cast(mainWidget.FindAnyWidget("ItemActionButton"));
		SelectBuyItem(mainButton, true);

		SilverTraderMenuBuyData mainParam = m_SilverBarter_BuyData.Get(id);
		if (!mainParam)
			return;

		float currentQty;
		if (m_SilverBarter_BuySelectedQuantities.Contains(mainParam.m_Classname))
			currentQty = m_SilverBarter_BuySelectedQuantities.Get(mainParam.m_Classname);
		else
			currentQty = Math.Min(1, mainParam.m_MaxBuyQuantity);

		float stepSize = pluginTrader.CalculateItemSelectedQuantityStep(mainParam.m_Classname);

		// Konsistente Step-Logik: im Sub-1-Bereich immer stepSize verwenden
		float actualStep = 0;
		if (stepSize < 1)
		{
			if (value > 0 && currentQty < 1)
				actualStep = stepSize;
			else if (value < 0 && currentQty <= 1)
				actualStep = stepSize * -1;
			else if (value > 0)
				actualStep = 1;
			else
				actualStep = -1;
		}
		else
		{
			actualStep = value;
		}

		float newQty = currentQty + actualStep;
		newQty = Math.Clamp(newQty, stepSize, mainParam.m_MaxBuyQuantity);
		m_SilverBarter_BuySelectedQuantities.Set(mainParam.m_Classname, newQty);

		UpdateItemInfoSelectedQuantity(mainWidget, mainParam.m_Classname, newQty, mainParam.m_MaxBuyQuantity);
		UpdateCurrentPriceProgress();
	}

	private void DoBarter()
	{
		if (m_SilverBarter_CurrentBarterProgress < 0)
			return;

		if (m_SilverBarter_BlockBarter)
			return;

		PluginSilverTrader pluginTrader = PluginSilverTrader.Cast(GetPlugin(PluginSilverTrader));
		if (!pluginTrader)
			return;

		map<string, float> buyItems = new map<string, float>;
		array<ItemBase> sellItems = new array<ItemBase>;

		GetSelectedSellItems(sellItems);
		if (sellItems.Count() > 0)
		{
			GetSelectedBuyItems(buyItems);

			int rotationRevision = 0;
			if (m_SilverBarter_TraderData)
				rotationRevision = m_SilverBarter_TraderData.m_rotationRevision;

			pluginTrader.DoBarter(m_SilverBarter_TraderId, sellItems, buyItems, rotationRevision);
		}

		sellItems = null;
		buyItems = null;
	}

	private string ReadSearchText(EditBoxWidget box)
	{
		string text = box.GetText();

		// "Use default text"-Verhalten von GetText() beim Placeholder ist nicht verifiziert - API pruefen.
		// Defensiv: literalen Placeholder-Text wie leeren String behandeln.
		if (text == "Suchen...")
			text = "";

		text.TrimInPlace();
		text.ToLower();
		return text;
	}

	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		super.OnChange(w, x, y, finished);

		if (w == m_SilverBarter_BuySearchBox)
		{
			m_SilverBarter_BuySearchText = ReadSearchText(EditBoxWidget.Cast(w));
			m_SilverBarter_PendingBuySearchTimer = 0.15;
			return true;
		}

		if (w == m_SilverBarter_SellSearchBox)
		{
			m_SilverBarter_SellSearchText = ReadSearchText(EditBoxWidget.Cast(w));
			m_SilverBarter_PendingSellSearchTimer = 0.25;
			return true;
		}

		return false;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);

		if (button == MouseState.LEFT)
		{
			if (w.GetUserID() == 1001)
			{
				SelectSellItem(ButtonWidget.Cast(w), true);
			}
			else if (w.GetUserID() == 1002)
			{
				SelectSellItem(ButtonWidget.Cast(w), false);
			}
			else if (w.GetUserID() == 2001)
			{
				SelectBuyItem(ButtonWidget.Cast(w), true);
			}
			else if (w.GetUserID() == 2002)
			{
				SelectBuyItem(ButtonWidget.Cast(w), false);
			}
			else if (w.GetUserID() == 3001)
			{
				ChangeBuyQuantity(ButtonWidget.Cast(w), -1);
			}
			else if (w.GetUserID() == 3002)
			{
				ChangeBuyQuantity(ButtonWidget.Cast(w), 1);
			}
			else if (w.GetUserID() >= 5000 && w.GetUserID() < 6000)
			{
				SwitchFilterItem(ButtonWidget.Cast(w));
			}
			else if (w == m_SilverBarter_BarterButton)
			{
				DoBarter();
			}
		}

		return false;
	}

	private void UpdateItemInfoDamage(Widget root_widget, ItemBase item)
	{
		int damageLevel = item.GetHealthLevel();

		if (damageLevel == GameConstants.STATE_RUINED)
		{
			WidgetTrySetText(root_widget, "ItemDamageWidget", "#inv_inspect_ruined", Colors.COLOR_RUINED);
		}
		else if (damageLevel == GameConstants.STATE_BADLY_DAMAGED)
		{
			WidgetTrySetText(root_widget, "ItemDamageWidget", "#inv_inspect_badly", Colors.COLOR_BADLY_DAMAGED);
		}
		else if (damageLevel == GameConstants.STATE_DAMAGED)
		{
			WidgetTrySetText(root_widget, "ItemDamageWidget", "#inv_inspect_damaged", Colors.COLOR_DAMAGED);
		}
		else if (damageLevel == GameConstants.STATE_WORN)
		{
			WidgetTrySetText(root_widget, "ItemDamageWidget", "#inv_inspect_worn", Colors.COLOR_WORN);
		}
		else if (damageLevel == GameConstants.STATE_PRISTINE)
		{
			WidgetTrySetText(root_widget, "ItemDamageWidget", "#inv_inspect_pristine", Colors.COLOR_PRISTINE);
		}
		else
		{
			WidgetTrySetText(root_widget, "ItemDamageWidget", "ERROR", Colors.COLOR_PRISTINE);
		}
	}

	private void UpdateItemInfoQuantity(Widget root_widget, ItemBase item_base)
	{
		PluginSilverTrader pluginTrader = PluginSilverTrader.Cast(GetPlugin(PluginSilverTrader));
		if (!pluginTrader)
			return;

		if (!pluginTrader.CanSellItem(m_SilverBarter_TraderInfo, item_base))
		{
			WidgetTrySetText(root_widget, "ItemQuantityWidget", "#silver_trader_block_sell", 0xFF800000);
			return;
		}

		float item_quantity = item_base.GetQuantity();
		int max_quantity = item_base.GetQuantityMax();

		if (max_quantity > 0 && !g_Game.ConfigIsExisting(CFG_VEHICLESPATH + " " + item_base.GetType() + " liquidContainerType"))
		{
			string quantity_str;
			float quantity_ratio;

			if (item_base.ConfigGetString("stackedUnit") == "pc." && item_base.CanBeSplit())
			{
				WidgetTrySetText(root_widget, "ItemQuantityWidget", item_quantity.ToString() + "/" + max_quantity.ToString() + " " + "#inv_inspect_pieces");
			}
			else if (item_base.ConfigGetString("stackedUnit") == "percentage")
			{
				quantity_ratio = Math.Round((item_quantity / max_quantity) * 100);
				quantity_str = quantity_ratio.ToString() + "#inv_inspect_percent";
				WidgetTrySetText(root_widget, "ItemQuantityWidget", quantity_str);
			}
			else if (item_base.ConfigGetString("stackedUnit") == "g")
			{
				quantity_ratio = Math.Round((item_quantity / max_quantity) * 100);
				quantity_str = quantity_ratio.ToString() + "#inv_inspect_percent";
				WidgetTrySetText(root_widget, "ItemQuantityWidget", quantity_str);
			}
			else if (item_base.ConfigGetString("stackedUnit") == "ml")
			{
				quantity_ratio = Math.Round((item_quantity / max_quantity) * 100);
				quantity_str = quantity_ratio.ToString() + "#inv_inspect_percent";
				WidgetTrySetText(root_widget, "ItemQuantityWidget", quantity_str);
			}
			else if (item_base.IsInherited(Ammunition_Base))
			{
				Magazine magazine_item;
				Class.CastTo(magazine_item, item_base);
				WidgetTrySetText(root_widget, "ItemQuantityWidget", magazine_item.GetAmmoCount().ToString() + "/" + magazine_item.GetAmmoMax().ToString() + " " + "#inv_inspect_pieces");
			}
			else if (item_base.IsInherited(Magazine))
			{
				WidgetTrySetText(root_widget, "ItemQuantityWidget", "");
			}
			else
			{
				WidgetTrySetText(root_widget, "ItemQuantityWidget", "");
			}
		}
		else
		{
			WidgetTrySetText(root_widget, "ItemQuantityWidget", "");
		}
	}

	private void UpdateItemInfoQuantity(Widget root_widget, PluginSilverTrader pluginTrader, string classname, float quantity)
	{
		if (!pluginTrader.CanBuyItem(m_SilverBarter_TraderInfo, classname))
		{
			WidgetTrySetText(root_widget, "ItemQuantityWidget", "#silver_trader_block_buy", 0xFF800000);
			return;
		}

		SilverItemConfigCache cache = pluginTrader.GetOrCreateItemCache(classname);

		if (cache.m_MaxStackSize > 0 && !cache.m_IsLiquidContainer)
		{
			float item_quantity;
			if (cache.m_StackedUnit == "pc." && cache.m_CanBeSplit)
			{
				item_quantity = quantity * cache.m_MaxStackSize;
				WidgetTrySetText(root_widget, "ItemQuantityWidget", FormatBuyQuantityStr(item_quantity) + " " + "#inv_inspect_pieces");
			}
			else if (cache.m_IsAmmo)
			{
				item_quantity = quantity * cache.m_MaxStackSize;
				WidgetTrySetText(root_widget, "ItemQuantityWidget", FormatBuyQuantityStr(item_quantity) + " " + "#inv_inspect_pieces");
			}
			else
			{
				WidgetTrySetText(root_widget, "ItemQuantityWidget", FormatBuyQuantityStr(quantity) + " " + "#inv_inspect_pieces");
			}
		}
		else
		{
			WidgetTrySetText(root_widget, "ItemQuantityWidget", FormatBuyQuantityStr(quantity) + " " + "#inv_inspect_pieces");
		}
	}

	private void UpdateItemInfoSelectedQuantity(Widget root_widget, string classname, float quantity, float maxQuantity)
	{
		PluginSilverTrader pluginTrader = PluginSilverTrader.Cast(GetPlugin(PluginSilverTrader));
		if (!pluginTrader)
			return;

		SilverItemConfigCache cache = pluginTrader.GetOrCreateItemCache(classname);

		if (cache.m_MaxStackSize > 0 && !cache.m_IsLiquidContainer)
		{
			float item_quantity;
			float item_max_quantity;
			if (cache.m_StackedUnit == "pc." && cache.m_CanBeSplit)
			{
				item_quantity = quantity * cache.m_MaxStackSize;
				item_max_quantity = maxQuantity * cache.m_MaxStackSize;
				WidgetTrySetText(root_widget, "ItemSelectedCountWidget", FormatBuyQuantityStr(item_quantity) + "/" + FormatBuyQuantityStr(item_max_quantity) + " " + "#inv_inspect_pieces");
			}
			else if (cache.m_IsAmmo)
			{
				item_quantity = quantity * cache.m_MaxStackSize;
				item_max_quantity = maxQuantity * cache.m_MaxStackSize;
				WidgetTrySetText(root_widget, "ItemSelectedCountWidget", FormatBuyQuantityStr(item_quantity) + "/" + FormatBuyQuantityStr(item_max_quantity) + " " + "#inv_inspect_pieces");
			}
			else
			{
				WidgetTrySetText(root_widget, "ItemSelectedCountWidget", FormatBuyQuantityStr(quantity) + "/" + FormatBuyQuantityStr(maxQuantity) + " " + "#inv_inspect_pieces");
			}
		}
		else
		{
			WidgetTrySetText(root_widget, "ItemSelectedCountWidget", FormatBuyQuantityStr(quantity) + "/" + FormatBuyQuantityStr(maxQuantity) + " " + "#inv_inspect_pieces");
		}
	}

	private void WidgetTrySetText(Widget root_widget, string widget_name, string text, int color = 0xFFFFFFFF)
	{
		TextWidget widget = TextWidget.Cast(root_widget.FindAnyWidget(widget_name));
		if (widget)
		{
			widget.SetText(text);
			widget.SetColor(color);
		}
	}

	private void WidgetSetWidth(Widget root_widget, string widget_name, float diff)
	{
		float w, h;
		Widget widget = root_widget.FindAnyWidget(widget_name);
		widget.GetSize(w, h);
		widget.SetSize(Math.Max(1, diff), h);
	}
};

class SilverTraderMenuBuyData
{
	string m_Classname;
	float m_TotalQuantity;
	float m_MaxBuyQuantity;
};
