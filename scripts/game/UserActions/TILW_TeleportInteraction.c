class TILW_TeleportInteraction : ScriptedUserAction
{
	[Attribute(uiwidget: UIWidgets.Auto, defvalue: "Teleport: Location", desc: "Action Name")]
	protected string m_actionName;

	[Attribute("", uiwidget: UIWidgets.Auto, desc: "Name of the entity to which the player will be teleported")]
	protected string m_locationName;

	[Attribute(uiwidget: UIWidgets.Auto, desc: "Allowed user faction keys. If empty, any.")]
	protected ref array<string> m_factionKeys;

	[Attribute("", uiwidget: UIWidgets.Auto, desc: "If defined, the interaction is only available if this mission flag is set")]
	protected string m_conditionFlag;

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity);
		if (playerId == 0)
			return;
		IEntity e = GetGame().GetWorld().FindEntityByName(m_locationName);
		if (!e) {
			Print("TILW_TeleportInteraction | Entity '" + m_locationName + "' could not be found!", LogLevel.ERROR);
			return;
		}
		SCR_Global.TeleportPlayer(playerId, e.GetOrigin());
	}
	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		outName = (m_actionName);
		return true;
	}
	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (m_conditionFlag == "")
			return true;

		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		return (fw && fw.GetMissionFlag(m_conditionFlag));
	}
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_factionKeys.IsEmpty()) {
			SCR_ChimeraCharacter cc = SCR_ChimeraCharacter.Cast(user);
			if (!cc || !m_factionKeys.Contains(cc.GetFactionKey()))
				return false;
		}
		return true;
	}
	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}
	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return true;
	}

}
