[ComponentEditorProps(category: "Game/", description: "When added to an entity with a SCR_DamageManagerComponent (e. g. a vehicle or character), reaching the target damage state (e. g. killed / destroyed, healed / repaired) will set a flag.")]
class TILW_Flag_EntityDamageClass : ScriptComponentClass
{
}
class TILW_Flag_EntityDamage : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.Auto, desc: "Flag that should be adjusted if the entities damage state matches the target state / cleared if it doesn't.")]
	string m_flagName;
	
	[Attribute(defvalue: "1", uiwidget: UIWidgets.Auto, desc: "Value that the flag should be adjusted by.")]
	int m_flagValue;
	
	[Attribute("2", UIWidgets.ComboBox, "Select target state for the selected hit zone (destroyed/killed, repaired/healed etc.)", enums: ParamEnumArray.FromEnum(EDamageState))]
	EDamageState m_targetState;
	
	[Attribute(defvalue: "", uiwidget: UIWidgets.Auto, desc: "Name of target hit zone. Optional.\nYou can look it up in the SCR_DamageManagerComponent of the prefab (or of e. g. its vehicle part prefabs).\nIf not set, the default hit zone (for vehicles, usually hull) is used, which is enough for e. g. basic Destroy/Kill tasks.")]
	string m_targetZone;
	
	[Attribute(defvalue: "0", uiwidget: UIWidgets.Auto, desc: "After the target state has been reached, keep listening for further changes and potentially remove the flag again.")]
	bool m_continuous;
	
	[Attribute(defvalue: "1", uiwidget: UIWidgets.Auto, desc: "What the initial health of the hit zone should be.", params: "0 1 0.01")]
	float m_initialHealth;
	
	EDamageState m_prevState;
	
	void TILW_Flag_EntityDamage(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		SetEventMask(ent, EntityEvent.INIT);
	}

	protected override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!Replication.IsServer())
			return;
		GetGame().GetCallqueue().Call(InitDamageFlag);
	}
	
	protected void InitDamageFlag()
	{
		SCR_HitZone hz = GetTargetHitZone();
		if (!hz)
			return;
		
		m_prevState = hz.GetDamageState();
		
		if (m_prevState == m_targetState)
		{
			TILW_MissionFrameworkEntity mfe = TILW_MissionFrameworkEntity.GetInstance();
			int newValue = mfe.GetMissionFlag(m_flagName) + m_flagValue;
			mfe.AdjustMissionFlag(m_flagName, newValue);
			if (!m_continuous)
				return;
		}
		
		hz.GetOnDamageStateChanged().Insert(OnDamageStateChangedHZ);
		if (m_initialHealth != 1.0)
			hz.SetHealthScaled(m_initialHealth);
		GetGame().GetCallqueue().Call(OnDamageStateChangedHZ);
	}
	
	protected void OnDamageStateChangedHZ()
	{
		EDamageState newState = GetTargetHitZone().GetDamageState();
		if (newState == m_targetState)
		{
			TILW_MissionFrameworkEntity mfe = TILW_MissionFrameworkEntity.GetInstance();
			int newValue = mfe.GetMissionFlag(m_flagName) + m_flagValue;
			mfe.AdjustMissionFlag(m_flagName, newValue);
			if (!m_continuous)
				GetTargetHitZone().GetOnDamageStateChanged().Remove(OnDamageStateChangedHZ);
		}
		else if (m_prevState == m_targetState)
		{
			TILW_MissionFrameworkEntity mfe = TILW_MissionFrameworkEntity.GetInstance();
			int newValue = mfe.GetMissionFlag(m_flagName) - m_flagValue;
			mfe.AdjustMissionFlag(m_flagName, newValue);
		}
		m_prevState = newState;
	}
	
	protected SCR_HitZone GetTargetHitZone()
	{
		SCR_DamageManagerComponent dmc = SCR_DamageManagerComponent.Cast(GetOwner().FindComponent(SCR_DamageManagerComponent));
		if (!dmc) {
			Print("TILW_Flag_EntityDamage | Failed to find SCR_DamageManagerComponent", LogLevel.ERROR);
			return null;
		}
		
		HitZone hitZone;
		if (m_targetZone != "")
			hitZone = dmc.GetHitZoneByName(m_targetZone);
		else
			hitZone = dmc.GetDefaultHitZone();
		
		if (!hitZone) {
			Print("TILW_Flag_EntityDamage | Failed to find '" + m_targetZone + "' HitZone - flag will not work.", LogLevel.ERROR);
			return null;
		}
		return SCR_HitZone.Cast(hitZone);
	}
	
}