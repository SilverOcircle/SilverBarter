// SilverBarter TraderPoint Entity
class TraderPoint extends BuildingSuper
{
	// Status
	bool m_ready = false;
	bool m_isRotatingTrader = false;

	// Server-Seite
	Object m_traderObject;

	// RPC Synch
	int m_traderId;
	int m_objectNetId1;
	int m_objectNetId2;

	void InitTraderPoint(int id, Object traderObj, bool isRotating = false)
	{
		m_traderId = id;
		m_traderObject = traderObj;
		m_isRotatingTrader = isRotating;
		m_ready = true;
	}

	bool IsTraderReady()
	{
		return m_ready;
	}

	int GetTraderId()
	{
		if (!m_ready)
			return -1;
		return m_traderId;
	}

	void OnTick()
	{
		if (!m_ready)
			return;

		if (m_traderObject)
		{
			m_traderObject.SetPosition(this.GetPosition());
			m_traderObject.SetOrientation(this.GetOrientation());
		}
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);

		if (m_traderObject)
		{
			g_Game.ObjectDelete(m_traderObject);
			m_traderObject = null;
		}
	}

	override void EEInit()
	{
		super.EEInit();

		// Client: Bei Init Server nach Trader-Infos fragen
		if (g_Game.IsClient())
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
			if (m_ready && m_traderObject)
			{
				int objNetId1;
				int objNetId2;
				m_traderObject.GetNetworkID(objNetId1, objNetId2);
				RPCSingleParam(SilverERPC.SILVERRPC_SYNCH_TRADER_POINT_CLIENT, new Param4<int, int, int, bool>(m_traderId, objNetId1, objNetId2, m_isRotatingTrader), true, sender);
			}
		}
		// Client: Trader-Infos vom Server empfangen
		else if (rpc_type == SilverERPC.SILVERRPC_SYNCH_TRADER_POINT_CLIENT)
		{
			Param4<int, int, int, bool> params;
			if (!ctx.Read(params))
				return;

			m_traderId = params.param1;
			m_objectNetId1 = params.param2;
			m_objectNetId2 = params.param3;
			m_isRotatingTrader = params.param4;
			m_ready = true;
		}
	}

	// Trader-Objekt holen (Server direkt, Client ueber Netzwerk-ID)
	Object GetTraderObject()
	{
		if (!m_ready)
			return null;

		// Server hat direkten Zugriff
		if (g_Game.IsServer() && m_traderObject)
		{
			return m_traderObject;
		}

		// Client holt ueber Netzwerk-ID
		return g_Game.GetObjectByNetworkId(m_objectNetId1, m_objectNetId2);
	}
};
