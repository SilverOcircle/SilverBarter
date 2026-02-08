// SilverBarter RPC-System
class SilverRPCManager
{
	// RPC Channel ID fuer SilverBarter
	static const int CHANNEL_SILVER_BARTER = 0x534C5652; // "SLVR"

	// Handler-Maps
	private static ref map<int, ref array<ref SilverRPCHandler>> s_handlers;
	private static bool s_initialized = false;

	static void Init()
	{
		if (s_initialized)
			return;

		s_handlers = new map<int, ref array<ref SilverRPCHandler>>;
		s_initialized = true;
	}

	static void RegisterHandler(int rpcType, Class instance, string methodName)
	{
		Init();

		if (!s_handlers.Contains(rpcType))
		{
			s_handlers.Insert(rpcType, new array<ref SilverRPCHandler>);
		}

		SilverRPCHandler handler = new SilverRPCHandler();
		handler.instance = instance;
		handler.methodName = methodName;
		s_handlers.Get(rpcType).Insert(handler);
	}

	static void SendToServer(int rpcType, Param params)
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!player)
			return;

		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(rpcType);
		rpc.Write(params);
		rpc.Send(player, CHANNEL_SILVER_BARTER, true);
	}

	static void SendToClient(int rpcType, PlayerIdentity identity, Param params)
	{
		if (!g_Game.IsServer())
			return;

		if (!identity)
			return;

		// Finde Player fuer diese Identity
		PlayerBase player = GetPlayerByIdentity(identity);
		if (!player)
			return;

		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(rpcType);
		rpc.Write(params);
		rpc.Send(player, CHANNEL_SILVER_BARTER, true, identity);
	}

	static void OnRPC(PlayerIdentity sender, ParamsReadContext ctx)
	{
		Init();

		int rpcType;
		if (!ctx.Read(rpcType))
			return;

		if (!s_handlers.Contains(rpcType))
			return;

		array<ref SilverRPCHandler> handlerList = s_handlers.Get(rpcType);
		foreach (SilverRPCHandler handler : handlerList)
		{
			if (handler && handler.instance)
			{
				g_Game.GameScript.CallFunctionParams(handler.instance, handler.methodName, null, new Param2<ParamsReadContext, PlayerIdentity>(ctx, sender));
			}
		}
	}

	private static PlayerBase GetPlayerByIdentity(PlayerIdentity identity)
	{
		if (!identity)
			return null;

		int lowBits;
		int highBits;
		g_Game.GetPlayerNetworkIDByIdentityID(identity.GetPlayerId(), lowBits, highBits);
		return PlayerBase.Cast(g_Game.GetObjectByNetworkId(lowBits, highBits));
	}
};

class SilverRPCHandler
{
	Class instance;
	string methodName;
};
