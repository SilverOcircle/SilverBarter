// SilverBarter Trader-Menu UI
class SilverTraderMenu extends UIScriptedMenu
{
	const float SELL_ITEM_DEPTH_OFFSET = 30;
	const float SELL_ITEM_HEIGHT_OFFSET = 2;
	const float PROGRESS_BAR_PRICE_DIVIDER = 25;

	bool m_active = false;
	bool m_dirty = false;

	int m_traderId;
	ref SilverTrader_Info m_traderInfo;
	ref SilverTrader_Data m_traderData;

	ref ScrollWidget m_sellItemsPanel;
	ref ScrollWidget m_buyItemsPanel;

	ref SimpleProgressBarWidget m_progressPositive;
	ref SimpleProgressBarWidget m_progressNegative;

	ref ButtonWidget m_barterBtn;
	ref MultilineTextWidget m_tradeButtonInfo;

	ref array<ref Widget> m_sellWidgetsCache;
	ref array<ref Widget> m_buyWidgetsCache;
	ref array<ref SilverTraderMenu_BuyData> m_buyData;
	ref array<EntityAI> m_previewItemsCache;

	ref array<string> m_filterData;
	static ref array<bool> m_filterMemory;

	float m_currentBarterProgress = 0;
	bool m_blockBarter = true;

	void SilverTraderMenu()
	{
		m_sellWidgetsCache = new array<ref Widget>;
		m_buyWidgetsCache = new array<ref Widget>;
		m_buyData = new array<ref SilverTraderMenu_BuyData>;
		m_previewItemsCache = new array<EntityAI>;
		m_filterData = new array<string>;

		if (!m_filterMemory)
		{
			m_filterMemory = new array<bool>;
		}
	}

	void InitMetadata(int traderId, SilverTrader_Info traderInfo, SilverTrader_Data traderData)
	{
		m_traderId = traderId;
		m_traderInfo = traderInfo;
		m_traderData = traderData;
		m_dirty = true;
	}

	void UpdateMetadata(SilverTrader_Data traderData)
	{
		if (m_traderData)
		{
			delete m_traderData;
		}
		m_traderData = traderData;
		m_dirty = true;
	}

	void CleanupBuyUI()
	{
		foreach (Widget w2 : m_buyWidgetsCache)
		{
			w2.Unlink();
		}
		m_buyWidgetsCache.Clear();

		foreach (SilverTraderMenu_BuyData buyData : m_buyData)
		{
			delete buyData;
		}
		m_buyData.Clear();

		foreach (EntityAI item : m_previewItemsCache)
		{
			g_Game.ObjectDelete(item);
		}
		m_previewItemsCache.Clear();
	}

	void CleanupUI()
	{
		CleanupBuyUI();

		foreach (Widget w1 : m_sellWidgetsCache)
		{
			w1.Unlink();
		}
		m_sellWidgetsCache.Clear();
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
		if (item)
		{
			nextItemIndex = InitItemSell(nextItemIndex + 1, 0, item, pluginTrader);
		}

		for (int i = 0; i < player.GetInventory().GetAttachmentSlotsCount(); ++i)
		{
			item = ItemBase.Cast(player.GetInventory().FindAttachment(player.GetInventory().GetAttachmentSlotId(i)));
			if (item)
			{
				nextItemIndex = InitItemSell(nextItemIndex + 1, 0, item, pluginTrader);
			}
		}
	}

	int InitItemSell(int index, int depth, ItemBase item, PluginSilverTrader pluginTrader)
	{
		int screenWidth;
		int screenHeight;
		GetScreenSize(screenWidth, screenHeight);

		Widget itemSell;
		if (screenHeight > 1440)
		{
			itemSell = g_Game.GetWorkspace().CreateWidgets("SilverBarter/layout/2160p/TraderMenuItemSell.layout");
		}
		else
		{
			itemSell = g_Game.GetWorkspace().CreateWidgets("SilverBarter/layout/TraderMenuItemSell.layout");
		}

		m_sellItemsPanel.AddChild(itemSell);

		float w, h;
		float contentWidth = m_sellItemsPanel.GetContentWidth() - (SELL_ITEM_DEPTH_OFFSET * depth);
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
		m_sellWidgetsCache.Insert(itemSell);

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
		m_buyItemsPanel.VScrollToPos01(0);

		PluginSilverTrader pluginTrader = PluginSilverTrader.Cast(GetPlugin(PluginSilverTrader));
		if (!pluginTrader)
			return;

		if (!m_traderData || !m_traderData.m_items)
			return;

		int nextItemIndex = -1;
		foreach (string classname, float quantity : m_traderData.m_items)
		{
			if (pluginTrader.FilterByCategories(m_filterData, m_filterMemory, classname))
			{
				ItemBase item = ItemBase.Cast(g_Game.CreateObject(classname, "0 0 0", true, false, false));
				if (item)
				{
					nextItemIndex = InitItemBuy(nextItemIndex + 1, item, classname, quantity, pluginTrader);
				}
			}
		}
	}

	int InitItemBuy(int index, ItemBase item, string classname, float quantity, PluginSilverTrader pluginTrader)
	{
		int screenWidth;
		int screenHeight;
		GetScreenSize(screenWidth, screenHeight);

		Widget itemBuy;
		if (screenHeight > 1440)
		{
			itemBuy = g_Game.GetWorkspace().CreateWidgets("SilverBarter/layout/2160p/TraderMenuItemBuy.layout");
		}
		else
		{
			itemBuy = g_Game.GetWorkspace().CreateWidgets("SilverBarter/layout/TraderMenuItemBuy.layout");
		}

		m_buyItemsPanel.AddChild(itemBuy);

		float w, h;
		float contentWidth = m_sellItemsPanel.GetContentWidth();
		itemBuy.GetSize(w, h);
		itemBuy.SetPos(0, (h + SELL_ITEM_HEIGHT_OFFSET) * index);
		itemBuy.SetSize(contentWidth, h);
		itemBuy.SetUserID(index);

		SilverTraderMenu_BuyData actionBtnParam = new SilverTraderMenu_BuyData;
		actionBtnParam.m_classname = classname;
		actionBtnParam.m_totalQuantity = quantity;
		actionBtnParam.m_maxBuyQuantity = Math.Min(pluginTrader.CalculateBuyMaxQuantity(m_traderInfo, classname), quantity);
		actionBtnParam.m_selectedQuantity = Math.Min(1, actionBtnParam.m_maxBuyQuantity);

		ButtonWidget actionButton = ButtonWidget.Cast(itemBuy.FindAnyWidget("ItemActionButton"));
		actionButton.SetUserID(2001);

		ItemPreviewWidget previewWidget = ItemPreviewWidget.Cast(itemBuy.FindAnyWidget("ItemPreviewWidget"));
		previewWidget.SetItem(item);
		previewWidget.SetView(item.GetViewIndex());
		previewWidget.SetModelPosition(Vector(0, 0, 1));
		m_previewItemsCache.Insert(item);

		WidgetSetWidth(itemBuy, "ItemNameWidget", contentWidth - 220);
		WidgetTrySetText(itemBuy, "ItemNameWidget", item.GetDisplayName());

		// Preis verstecken - Balken-System zeigt Tauschwert
		WidgetTrySetText(itemBuy, "ItemPriceWidget", " ");

		UpdateItemInfoQuantity(itemBuy, pluginTrader, classname, quantity);
		UpdateItemInfoSelectedQuantity(itemBuy, classname, actionBtnParam.m_selectedQuantity, actionBtnParam.m_maxBuyQuantity);

		ButtonWidget minusButton = ButtonWidget.Cast(itemBuy.FindAnyWidget("MinusActionBtn"));
		minusButton.SetUserID(3001);

		ButtonWidget plusButton = ButtonWidget.Cast(itemBuy.FindAnyWidget("PlusActionBtn"));
		plusButton.SetUserID(3002);

		m_buyWidgetsCache.Insert(itemBuy);
		m_buyData.Insert(actionBtnParam);
		return index;
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

			if (pluginTrader.CanSellItem(m_traderInfo, sellItem))
			{
				value = value + pluginTrader.CalculateSellPrice(m_traderInfo, m_traderData, sellItem);
			}
			else
			{
				blockedItemsCounter++;
			}
		}
		delete sellResult;

		map<string, float> buyResult = new map<string, float>;
		GetSelectedBuyItems(buyResult);
		foreach (string buyClassname, float buyQuantity : buyResult)
		{
			if (pluginTrader.CanBuyItem(m_traderInfo, buyClassname))
			{
				value = value - pluginTrader.CalculateBuyPrice(m_traderInfo, m_traderData, buyClassname, buyQuantity);
			}
			else
			{
				blockedItemsCounter++;
			}
		}
		delete buyResult;

		value = value / PROGRESS_BAR_PRICE_DIVIDER;

		if (value > 0)
		{
			m_progressPositive.SetCurrent(Math.Min(100, value));
			m_progressNegative.SetCurrent(0);
			m_barterBtn.Enable(true);
		}
		else if (value < 0)
		{
			m_progressPositive.SetCurrent(0);
			m_progressNegative.SetCurrent(Math.Min(100, value * -1));
			m_barterBtn.Enable(false);
		}
		else
		{
			m_progressPositive.SetCurrent(0);
			m_progressNegative.SetCurrent(0);
			m_barterBtn.Enable(true);
		}

		if (blockedItemsCounter > 0)
		{
			m_tradeButtonInfo.SetText("#silver_trader_block_baditems");
			m_blockBarter = true;
		}
		else if (pluginTrader.HasOversizedSellItems(m_traderInfo, m_traderData, sellCounter))
		{
			m_tradeButtonInfo.SetText("#silver_trader_block_toomany");
			m_blockBarter = true;
		}
		else
		{
			m_tradeButtonInfo.SetText("");
			m_blockBarter = false;
		}

		delete sellCounter;
		m_currentBarterProgress = value;
	}

	void GetSelectedSellItems(array<ItemBase> result)
	{
		foreach (Widget w : m_sellWidgetsCache)
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
		int index;
		SilverTraderMenu_BuyData buyData;
		foreach (Widget w : m_buyWidgetsCache)
		{
			Widget btn = w.FindAnyWidget("ItemActionButton");
			if (btn.GetUserID() == 2002)
			{
				index = w.GetUserID();
				buyData = m_buyData.Get(index);
				result.Insert(buyData.m_classname, buyData.m_selectedQuantity);
			}
		}
	}

	void InitializeFilter(Widget root, string name)
	{
		int id = m_filterData.Insert(name);
		ButtonWidget btn = ButtonWidget.Cast(root.FindAnyWidget("FilterActionBtn" + id));
		btn.SetUserID(5000 + id);

		TextWidget btnText = TextWidget.Cast(btn.GetChildren());
		btnText.SetText("#silver_trader_filter_" + name);

		if (m_filterMemory.Count() <= id)
		{
			m_filterMemory.Insert(true);
		}

		SelectFilterItem(btn, m_filterMemory.Get(id));
	}

	override Widget Init()
	{
		int screenWidth;
		int screenHeight;
		GetScreenSize(screenWidth, screenHeight);

		if (screenHeight > 1440)
		{
			layoutRoot = g_Game.GetWorkspace().CreateWidgets("SilverBarter/layout/2160p/TraderMenu.layout");
		}
		else
		{
			layoutRoot = g_Game.GetWorkspace().CreateWidgets("SilverBarter/layout/TraderMenu.layout");
		}

		m_sellItemsPanel = ScrollWidget.Cast(layoutRoot.FindAnyWidget("SellItemsPanel"));
		m_buyItemsPanel = ScrollWidget.Cast(layoutRoot.FindAnyWidget("BuyItemsPanel"));
		m_progressPositive = SimpleProgressBarWidget.Cast(layoutRoot.FindAnyWidget("ProgressPositive"));
		m_progressNegative = SimpleProgressBarWidget.Cast(layoutRoot.FindAnyWidget("ProgressNegative"));
		m_barterBtn = ButtonWidget.Cast(layoutRoot.FindAnyWidget("TradeButton"));
		m_tradeButtonInfo = MultilineTextWidget.Cast(layoutRoot.FindAnyWidget("TradeButtonInfo"));

		m_filterData.Clear();
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

		m_active = true;
		return layoutRoot;
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);

		if (m_dirty)
		{
			CleanupUI();
			InitInventorySell();
			InitInventoryBuy();
			UpdateCurrentPriceProgress();
			m_dirty = false;
		}

		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!player || !player.IsAlive() || player.IsUnconscious() || player.IsRestrained())
		{
			m_active = false;
		}
		else if (m_traderInfo && vector.Distance(m_traderInfo.m_position, player.GetPosition()) > 5)
		{
			m_active = false;
		}

		if (!m_active)
		{
			g_Game.GetUIManager().Back();
		}
		else
		{
			// Focus wiederherstellen nach Alt+Tab
			g_Game.GetInput().ChangeGameFocus(1);
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
			closeRpc.Write(m_traderId);
			closeRpc.Send(closePlayer, SilverRPCManager.CHANNEL_SILVER_BARTER, true);
			Print("[SilverBarter] Close RPC sent for trader " + m_traderId.ToString());
		}

		CleanupUI();

		if (m_traderInfo)
		{
			delete m_traderInfo;
		}

		if (m_traderData)
		{
			delete m_traderData;
		}

		if (m_buyData)
			delete m_buyData;
		if (m_sellWidgetsCache)
			delete m_sellWidgetsCache;
		if (m_buyWidgetsCache)
			delete m_buyWidgetsCache;
		if (m_filterData)
			delete m_filterData;

		Close();
	}

	private void SelectSellItem(ButtonWidget btn, bool enable)
	{
		Widget back = btn.GetParent();
		if (!back)
			return;

		int depth = back.GetUserID();
		int itemId = m_sellWidgetsCache.Find(back.GetParent());

		if (itemId != -1)
		{
			int index = itemId - 1;

			if (btn.GetUserID() == 1002)
			{
				while (index >= 0)
				{
					Widget prevItem = m_sellWidgetsCache.Get(index);
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
			while (index < m_sellWidgetsCache.Count())
			{
				Widget nextItem = m_sellWidgetsCache.Get(index);
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
		m_filterMemory.Set(btn.GetUserID() - 5000, enable);

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

		ButtonWidget mainButton = ButtonWidget.Cast(mainWidget.FindAnyWidget("ItemActionButton"));
		SelectBuyItem(mainButton, true);

		SilverTraderMenu_BuyData mainParam = m_buyData.Get(id);

		float stepSize = pluginTrader.CalculateItemSelectedQuantityStep(mainParam.m_classname);
		if (value < 0 && mainParam.m_selectedQuantity <= 1)
		{
			value = stepSize * -1;
		}
		else if (value > 0 && mainParam.m_selectedQuantity < 1)
		{
			value = stepSize;
		}

		if ((mainParam.m_selectedQuantity + value) > 0)
		{
			mainParam.m_selectedQuantity = mainParam.m_selectedQuantity + value;
			mainParam.m_selectedQuantity = Math.Min(mainParam.m_selectedQuantity, mainParam.m_maxBuyQuantity);
		}

		// Preis verstecken - Balken-System zeigt Tauschwert
		UpdateItemInfoSelectedQuantity(mainWidget, mainParam.m_classname, mainParam.m_selectedQuantity, mainParam.m_maxBuyQuantity);
		UpdateCurrentPriceProgress();
	}

	private void DoBarter()
	{
		if (m_currentBarterProgress < 0)
			return;

		if (m_blockBarter)
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
			pluginTrader.DoBarter(m_traderId, sellItems, buyItems);
		}

		delete sellItems;
		delete buyItems;
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
			else if (w == m_barterBtn)
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

		if (!pluginTrader.CanSellItem(m_traderInfo, item_base))
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
		if (!pluginTrader.CanBuyItem(m_traderInfo, classname))
		{
			WidgetTrySetText(root_widget, "ItemQuantityWidget", "#silver_trader_block_buy", 0xFF800000);
			return;
		}

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

		int maxStacksCount = pluginTrader.CalculateTraderItemQuantityMax(m_traderInfo, classname);

		// canBeSplit pruefen: nur echte Stacks multiplizieren Quantity mit maxStackSize
		int canBeSplit = 0;
		if (g_Game.ConfigIsExisting(CFG_VEHICLESPATH + " " + classname + " canBeSplit"))
		{
			canBeSplit = g_Game.ConfigGetInt(CFG_VEHICLESPATH + " " + classname + " canBeSplit");
		}

		if (maxStackSize > 0 && !g_Game.ConfigIsExisting(CFG_VEHICLESPATH + " " + classname + " liquidContainerType"))
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

			float item_quantity;

			if (stackedUnits == "pc." && canBeSplit == 1)
			{
				item_quantity = quantity * maxStackSize;
				WidgetTrySetText(root_widget, "ItemQuantityWidget", FormatBuyQuantityStr(item_quantity) + " " + "#inv_inspect_pieces");
			}
			else if (g_Game.IsKindOf(classname, "Ammunition_Base"))
			{
				item_quantity = quantity * maxStackSize;
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

		int canBeSplit = 0;
		if (g_Game.ConfigIsExisting(CFG_VEHICLESPATH + " " + classname + " canBeSplit"))
		{
			canBeSplit = g_Game.ConfigGetInt(CFG_VEHICLESPATH + " " + classname + " canBeSplit");
		}

		if (maxStackSize > 0 && !g_Game.ConfigIsExisting(CFG_VEHICLESPATH + " " + classname + " liquidContainerType"))
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

			float item_quantity;
			float item_max_quantity;

			if (stackedUnits == "pc." && canBeSplit == 1)
			{
				item_quantity = quantity * maxStackSize;
				item_max_quantity = maxQuantity * maxStackSize;
				WidgetTrySetText(root_widget, "ItemSelectedCountWidget", FormatBuyQuantityStr(item_quantity) + "/" + FormatBuyQuantityStr(item_max_quantity) + " " + "#inv_inspect_pieces");
			}
			else if (g_Game.IsKindOf(classname, "Ammunition_Base"))
			{
				item_quantity = quantity * maxStackSize;
				item_max_quantity = maxQuantity * maxStackSize;
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

class SilverTraderMenu_BuyData
{
	string m_classname;
	float m_totalQuantity;
	float m_selectedQuantity;
	float m_maxBuyQuantity;
};
