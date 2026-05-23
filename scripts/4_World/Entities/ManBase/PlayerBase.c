// SilverBarter Player-Erweiterung
modded class PlayerBase
{
	override void SetActions(out TInputActionMap InputActionMap)
	{
		super.SetActions(InputActionMap);
		AddAction(ActionTraderInteract, InputActionMap);
	}

	// RPC-Hook fuer SilverBarter
	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);

		if (rpc_type == SilverRPCManager.CHANNEL_SILVER_BARTER)
		{
			#ifdef DEVELOPER
			Print("[SilverBarter] PlayerBase.OnRPC: RPC received");
			#endif
			SilverRPCManager.OnRPC(sender, ctx);
		}
	}
};
