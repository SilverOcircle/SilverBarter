class SilverTraderInspectMenu extends InspectMenuNew
{
	override void OnHide()
	{
		super.OnHide();
		PPERequesterBank.GetRequester(PPERequester_InventoryBlur).Stop();

		Mission mission = g_Game.GetMission();
		if (mission && mission.GetHud())
		{
			mission.GetHud().ShowHudUI(true);
			mission.GetHud().ShowQuickbarUI(true);
		}
	}
}
