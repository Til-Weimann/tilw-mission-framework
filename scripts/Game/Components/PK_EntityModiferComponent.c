[ComponentEditorProps(category: "Game/", description: "Enable different modifiers that can be used on entities/prefabs")]
class PK_EntityModiferComponentClass : ScriptComponentClass
{
}
class PK_EntityModiferComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.CheckBox, "Damage Manager enabled/disable", category: "Damage Manager")]
	protected bool m_damageManagerEnabled;

	[Attribute("", UIWidgets.Auto, "", desc: "Damage list", category: "Damage Manager")]
	protected ref array<ref PK_EntitesDamageList> m_EntitiesDamageList;
	
	[Attribute("0", UIWidgets.CheckBox, "Engine Manager enabled/disabled", category: "Engine Manager")]
	protected bool m_engineEnabled;
	
	[Attribute("0", UIWidgets.CheckBox, "Light Manager enabled/disabled", category: "Light Manager")]
	protected bool m_lightEnabled;
	
	[Attribute("", UIWidgets.Auto, "", desc: "List of lights to enable", category: "Light Manager")]
	protected ref array<ref PK_LightList> m_LightList;
	
	override void OnPostInit(IEntity owner)
	{
		if (!Replication.IsServer())
			return;
		if(m_damageManagerEnabled)
			GetGame().GetCallqueue().Call(InitDamageFlag);
		if(m_engineEnabled)
			GetGame().GetCallqueue().Call(InitStartEngine);
		if(m_lightEnabled)
			GetGame().GetCallqueue().Call(InitLightManager);

	}
	
	
	
	protected void InitStartEngine()
	{
		BaseVehicleControllerComponent m_EngineAction = BaseVehicleControllerComponent.Cast(GetOwner().FindComponent(BaseVehicleControllerComponent));
		m_EngineAction.ForceStartEngine();	
	}
	
	
	protected void InitLightManager()
	{

		BaseLightManagerComponent vehicleLightManagerComp = BaseLightManagerComponent.Cast(GetOwner().FindComponent(BaseLightManagerComponent));
		foreach (PK_LightList m_light : m_LightList)
		{
			if(!m_light.m_enabled)
				continue;
			vehicleLightManagerComp.SetLightsState(m_light.m_eLightType, true, m_light.m_iLightSide);

		}
	}
	
	// Damage functions
	protected void InitDamageFlag()
	{
		foreach (PK_EntitesDamageList m_EntitieDamage : m_EntitiesDamageList)
		{
			if(!m_EntitieDamage.m_enabled)
				continue;
			
			SCR_HitZone hz = GetTargetHitZone(m_EntitieDamage.m_targetZone);
				if (!hz)
				continue;

			if (m_EntitieDamage.m_initialHealth != 1.0)
				hz.SetHealthScaled(m_EntitieDamage.m_initialHealth);
		}
	}
	

	protected SCR_HitZone GetTargetHitZone(string m_targetZone)
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




[BaseContainerProps()]
class PK_LightList
{
	//! Type of action instead of 8 different classes
	[Attribute(defvalue: SCR_Enum.GetDefault(ELightType.Head), uiwidget: UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(ELightType))]
	ELightType m_eLightType;

	//! Side of turn signals
	[Attribute("-1", uiwidget: UIWidgets.ComboBox, enums: { ParamEnum("Either", "-1"), ParamEnum("Left", "0"), ParamEnum("Right", "1")})]
	int m_iLightSide;
	
	[Attribute("1", UIWidgets.CheckBox, "Enable/Disable this part. Great for testing or debugging")]
	bool m_enabled;
}


[BaseContainerProps()]
class PK_EntitesDamageList
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.Auto, desc: "Name of target hit zone. Optional.\nYou can look it up in the SCR_DamageManagerComponent of the prefab (or of e. g. its vehicle part prefabs).\nIf not set, the default hit zone (for vehicles, usually hull) is used, which is enough for e. g. basic Destroy/Kill tasks.", category: "Damage Manager")]
	string m_targetZone;
	[Attribute("100", UIWidgets.Slider, params: "0 100 1", desc: "What the initial health of the hit zone should be. [% of health]", category: "Damage Manager")]
	float m_initialHealth;
	[Attribute("1", UIWidgets.CheckBox, "Enable/Disable this part. Great for testing or debugging", category: "Damage Manager")]
	bool m_enabled;
}