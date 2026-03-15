[ComponentEditorProps(category: "Game/", description: "")]
class TILW_ControlModeFlagClass : ScriptComponentClass
{
}
class TILW_ControlModeFlag : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.Auto, desc: "Flag that should be adjusted if the owner AI groups control mode matches the target mode.")]
	protected string m_flagName;
	
	[Attribute(defvalue: "1", uiwidget: UIWidgets.Auto, desc: "Value that the flag should be adjusted by.")]
	protected int m_flagValue;
	
	[Attribute("2", UIWidgets.ComboBox, "Select target control mode - autonomous = combat movement.\nNote that with some waypoints like defend, individual soliders would engage in combat while the group stays in FOLLOWING_WAYPOINT.", enums: ParamEnumArray.FromEnum(EGroupControlMode))]
	protected EGroupControlMode m_targetControlMode;
	
	[Attribute(defvalue: "0", uiwidget: UIWidgets.Auto, desc: "After the target mode has been reached, keep listening for further changes and potentially remove the flag again.\nNote that every group starts at NONE (pre initialization), then goes to IDLE before doing anything else happens.")]
	protected bool m_continuous;
	
	void TILW_ControlModeFlag(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		SetEventMask(ent, EntityEvent.INIT);
	}

	protected override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!Replication.IsServer())
			return;
		GetGame().GetCallqueue().Call(InitFlag);
	}
	
	protected void InitFlag()
	{
		SCR_AIGroupInfoComponent groupInfo = SCR_AIGroupInfoComponent.Cast(GetOwner().FindComponent(SCR_AIGroupInfoComponent));
		groupInfo.GetOnControlModeChanged().Insert(OnControlModeChanged);
		GetGame().GetCallqueue().Call(OnControlModeChanged, groupInfo.GetGroupControlMode());
	}
	
	protected void OnControlModeChanged(EGroupControlMode mode)
	{
		int adjustValue = m_flagValue;
		if (mode != m_targetControlMode)
			adjustValue = -m_flagValue;

		TILW_MissionFrameworkEntity mfe = TILW_MissionFrameworkEntity.GetInstance();
		if (mfe)
		{
			int newValue = mfe.GetMissionFlag(m_flagName) + adjustValue;
			mfe.AdjustMissionFlag(m_flagName, newValue);
		}
		if (!m_continuous && mode == m_targetControlMode)
		{
			SCR_AIGroupInfoComponent groupInfo = SCR_AIGroupInfoComponent.Cast(GetOwner().FindComponent(SCR_AIGroupInfoComponent));
			groupInfo.GetOnControlModeChanged().Remove(OnControlModeChanged);
		}
	}
	
}