// SilverBarter Plugin-Registrierung
modded class PluginManager
{
	override void Init()
	{
		RegisterPlugin("PluginSilverTrader", true, true);
		super.Init();
	}
};
