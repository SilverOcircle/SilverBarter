// SilverBarter Server Mission Init
modded class MissionServer
{
	override void OnMissionStart()
	{
		super.OnMissionStart();

		PluginSilverTrader traderPlugin = PluginSilverTrader.Cast(GetPlugin(PluginSilverTrader));
		if (traderPlugin)
		{
			traderPlugin.InitializeTraders();
		}
	}
};
