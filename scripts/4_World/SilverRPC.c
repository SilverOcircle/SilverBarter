// SilverBarter RPC-System
class SilverRPCManager
{
	// RPC Channel ID fuer SilverBarter
	static const int CHANNEL_SILVER_BARTER = 0x534C5652; // "SLVR"

	// Handler-Maps
	private static ref map<int, ref array<ref SilverRPCHandler>> s_Handlers;
	private static bool s_Initialized = false;

	static void Init()
	{
		if (s_Initialized)
			return;

		s_Handlers = new map<int, ref array<ref SilverRPCHandler>>;
		s_Initialized = true;
	}

	static void RegisterHandler(int rpcType, Class instance, string methodName)
	{
		Init();

		// Alte Handler fuer diesen rpcType entfernen (verhindert Dangling Pointer nach Reconnect)
		if (s_Handlers.Contains(rpcType))
		{
			s_Handlers.Get(rpcType).Clear();
		}
		else
		{
			s_Handlers.Insert(rpcType, new array<ref SilverRPCHandler>);
		}

		SilverRPCHandler handler = new SilverRPCHandler();
		handler.m_Instance = instance;
		handler.m_MethodName = methodName;
		s_Handlers.Get(rpcType).Insert(handler);
	}

	static void UnregisterInstance(Class instance)
	{
		if (!instance || !s_Handlers)
			return;

		foreach (int rpcType, array<ref SilverRPCHandler> handlerList : s_Handlers)
		{
			for (int i = handlerList.Count() - 1; i >= 0; i--)
			{
				SilverRPCHandler handler = handlerList.Get(i);
				if (!handler || handler.m_Instance == instance)
					handlerList.Remove(i);
			}
		}
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
		if (!g_Game || !g_Game.IsDedicatedServer())
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

		if (!s_Handlers.Contains(rpcType))
			return;

		array<ref SilverRPCHandler> handlerList = s_Handlers.Get(rpcType);
		foreach (SilverRPCHandler handler : handlerList)
		{
			if (handler && handler.m_Instance)
			{
				g_Game.GameScript.CallFunctionParams(handler.m_Instance, handler.m_MethodName, null, new Param2<ParamsReadContext, PlayerIdentity>(ctx, sender));
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
	Class m_Instance;
	string m_MethodName;
};
