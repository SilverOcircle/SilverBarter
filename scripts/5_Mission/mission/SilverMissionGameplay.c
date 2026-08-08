// SilverBarter Client Mission (ESC-Handler)
modded class MissionGameplay
{
	override UIScriptedMenu CreateScriptedMenu(int id)
	{
		if (id == SILVER_MENU_ITEM_INSPECT)
			return new SilverTraderInspectMenu;

		return super.CreateScriptedMenu(id);
	}

	override void OnKeyPress(int key)
	{
		if (key == KeyCode.KC_ESCAPE)
		{
			UIScriptedMenu inspectMenu = UIScriptedMenu.Cast(g_Game.GetUIManager().FindMenu(SILVER_MENU_ITEM_INSPECT));
			if (inspectMenu)
			{
				inspectMenu.Close();
				return;
			}
		}

		super.OnKeyPress(key);

		if (key == KeyCode.KC_ESCAPE)
		{
			PluginSilverTrader traderPlugin;
			Class.CastTo(traderPlugin, GetPlugin(PluginSilverTrader));
			if (traderPlugin)
			{
				traderPlugin.CloseTraderMenu();
			}
		}
	}
};
