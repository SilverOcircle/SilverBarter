// SilverBarter Client Mission (ESC-Handler)
modded class MissionGameplay
{
	override void OnKeyPress(int key)
	{
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
