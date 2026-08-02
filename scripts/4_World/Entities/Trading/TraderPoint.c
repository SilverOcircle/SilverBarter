// SilverBarter TraderPoint Entity
class TraderPoint extends BuildingSuper
{
	private static ref array<TraderPoint> s_SilverBarter_Registry;

	// Status
	bool m_SilverBarter_Ready = false;
	bool m_SilverBarter_IsRotatingTrader = false;

	// Server-Seite
	Object m_SilverBarter_TraderObject;

	// RPC Synch
	int m_SilverBarter_TraderId;
	int m_SilverBarter_ObjectNetId1;
	int m_SilverBarter_ObjectNetId2;

	void InitTraderPoint(int id, Object traderObj, bool isRotating = false)
	{
		m_SilverBarter_TraderId = id;
		m_SilverBarter_TraderObject = traderObj;
		m_SilverBarter_IsRotatingTrader = isRotating;
		m_SilverBarter_Ready = true;

		if (m_SilverBarter_TraderObject)
		{
			m_SilverBarter_TraderObject.SetPosition(GetPosition());
			m_SilverBarter_TraderObject.SetOrientation(GetOrientation());
		}
	}

	bool IsTraderReady()
	{
		return m_SilverBarter_Ready;
	}

	int GetTraderId()
	{
		if (!m_SilverBarter_Ready)
			return -1;
		return m_SilverBarter_TraderId;
	}

	static TraderPoint FindByTraderObject(Object traderObject)
	{
		if (!traderObject || !s_SilverBarter_Registry)
			return null;

		foreach (TraderPoint traderPoint : s_SilverBarter_Registry)
		{
			if (traderPoint && traderPoint.IsTraderReady() && traderPoint.GetTraderObject() == traderObject)
				return traderPoint;
		}

		return null;
	}

	override void EEDelete(EntityAI parent)
	{
		if (s_SilverBarter_Registry)
			s_SilverBarter_Registry.RemoveItem(this);

		CGame game = g_Game;
		if (game && game.IsDedicatedServer() && m_SilverBarter_TraderObject)
		{
			game.ObjectDelete(m_SilverBarter_TraderObject);
			m_SilverBarter_TraderObject = null;
		}

		super.EEDelete(parent);
	}

	override void EEInit()
	{
		super.EEInit();

		if (!s_SilverBarter_Registry)
			s_SilverBarter_Registry = new array<TraderPoint>;
		if (s_SilverBarter_Registry.Find(this) == -1)
			s_SilverBarter_Registry.Insert(this);

		// Client: Bei Init Server nach Trader-Infos fragen
		if (g_Game && !g_Game.IsDedicatedServer())
		{
			RPCSingleParam(SilverERPC.SILVERRPC_SYNCH_TRADER_POINT_SERVER, new Param1<int>(0), true);
		}
	}

	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);

		// Server: Client fragt nach Trader-Infos
		if (rpc_type == SilverERPC.SILVERRPC_SYNCH_TRADER_POINT_SERVER)
		{
			if (!g_Game || !g_Game.IsDedicatedServer() || !sender)
				return;

			if (m_SilverBarter_Ready && m_SilverBarter_TraderObject)
			{
				int objNetId1;
				int objNetId2;
				m_SilverBarter_TraderObject.GetNetworkID(objNetId1, objNetId2);
				RPCSingleParam(SilverERPC.SILVERRPC_SYNCH_TRADER_POINT_CLIENT, new Param4<int, int, int, bool>(m_SilverBarter_TraderId, objNetId1, objNetId2, m_SilverBarter_IsRotatingTrader), true, sender);
			}
		}
		// Client: Trader-Infos vom Server empfangen
		else if (rpc_type == SilverERPC.SILVERRPC_SYNCH_TRADER_POINT_CLIENT)
		{
			if (!g_Game || g_Game.IsDedicatedServer())
				return;

			Param4<int, int, int, bool> params;
			if (!ctx.Read(params))
				return;

			m_SilverBarter_TraderId = params.param1;
			m_SilverBarter_ObjectNetId1 = params.param2;
			m_SilverBarter_ObjectNetId2 = params.param3;
			m_SilverBarter_IsRotatingTrader = params.param4;
			m_SilverBarter_Ready = true;
		}
	}

	// Trader-Objekt holen (Server direkt, Client ueber Netzwerk-ID)
	Object GetTraderObject()
	{
		if (!m_SilverBarter_Ready)
			return null;

		// Server hat direkten Zugriff
		if (g_Game && g_Game.IsDedicatedServer() && m_SilverBarter_TraderObject)
		{
			return m_SilverBarter_TraderObject;
		}

		// Client holt ueber Netzwerk-ID
		if (!g_Game)
			return null;

		return g_Game.GetObjectByNetworkId(m_SilverBarter_ObjectNetId1, m_SilverBarter_ObjectNetId2);
	}
};
