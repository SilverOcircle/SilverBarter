// SilverBarter Trader-Interaktions-Action
class ActionTraderInteract: ActionInteractBase
{
	void ActionTraderInteract()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
		m_HUDCursorIcon = CursorIcons.CloseHood;
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINone;
		m_ConditionTarget = new CCTObject;
	}

	override string GetText()
	{
		return "#silver_action_trade";
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		// Server gibt immer true zurueck (Validierung erfolgt spaeter)
		if (GetGame().IsServer())
			return true;

		// Client-Pruefungen
		if (g_Game.GetUIManager().GetMenu() != null)
			return false;

		if (!target || !target.GetObject())
			return false;

		TraderPoint traderPoint = FindTraderPoint(target.GetObject().GetPosition());
		if (traderPoint && traderPoint.IsTraderReady() && traderPoint.GetTraderObject() == target.GetObject())
		{
			return true;
		}

		return false;
	}

	// TraderPoint in der Naehe finden
	TraderPoint FindTraderPoint(vector pos)
	{
		TraderPoint result = null;
		ref array<Object> objects = new array<Object>;
		g_Game.GetObjectsAtPosition3D(pos, 0.1, objects, null);

		foreach (Object obj : objects)
		{
			if (obj.IsInherited(TraderPoint))
			{
				result = TraderPoint.Cast(obj);
				break;
			}
		}

		delete objects;
		return result;
	}

	override void OnEndServer(ActionData action_data)
	{
		if (!action_data.m_Player || !action_data.m_Target || !action_data.m_Target.GetObject())
			return;

		TraderPoint traderPoint = FindTraderPoint(action_data.m_Target.GetObject().GetPosition());
		if (!traderPoint || !traderPoint.IsTraderReady())
			return;

		if (traderPoint.GetTraderObject() != action_data.m_Target.GetObject())
			return;

		PluginSilverTrader traderPlugin = PluginSilverTrader.Cast(GetPlugin(PluginSilverTrader));
		if (traderPlugin)
		{
			traderPlugin.SendTraderMenuOpen(action_data.m_Player, traderPoint.GetTraderId());
		}
	}
};
