[ComponentEditorProps(category: "Game/", description: "Enable different modifiers that can be used on entities/prefabs")]
class PK_EntityModifierComponentClass : ScriptComponentClass
{
}
class PK_EntityModifierComponent : ScriptComponent
{
	
	[Attribute("0", UIWidgets.CheckBox, "Damage Manager enabled/disable", category: "Damage Manager")]
	protected bool m_bDamageManagerEnabled;

	[Attribute("", UIWidgets.Auto, "", desc: "Damage list", category: "Damage Manager")]
	protected ref array<ref PK_HitzoneData> m_aHitzonesData;

	[Attribute("0", UIWidgets.CheckBox, "Debug output all hitzones by name and index. Filter for PK_Hitzones. Currently index is only used for wheels", category: "Damage Manager")]
	protected bool m_bDebugPrintAllHitZones;
	
	
	[Attribute("0", UIWidgets.CheckBox, "Engine Manager enabled/disabled", category: "Engine Manager")]
	protected bool m_bEngineEnabled;
	
	[Attribute("0", UIWidgets.CheckBox, "Light Manager enabled/disabled", category: "Light Manager")]
	protected bool m_bLightEnabled;
	
	[Attribute("", UIWidgets.Auto, "", desc: "List of lights to enable", category: "Light Manager")]
	protected ref array<ref PK_LightData> m_aLightsData;

	override void OnPostInit(IEntity owner)
	{
		if (!Replication.IsServer())
			return;
		if(m_bDamageManagerEnabled)
			GetGame().GetCallqueue().Call(SetInitialHitZoneHealth);
		if(m_bEngineEnabled)
			GetGame().GetCallqueue().Call(SetInitialEngineState);
		if(m_bLightEnabled)
			GetGame().GetCallqueue().Call(SetInitialLightStates);
		if(m_bDebugPrintAllHitZones)
			GetGame().GetCallqueue().Call(DebugPrintAllHitZones);
	}
	
	protected void SetInitialEngineState()
	{
		BaseVehicleControllerComponent vehicleController = BaseVehicleControllerComponent.Cast(GetOwner().FindComponent(BaseVehicleControllerComponent));
		if (!vehicleController) {
			Print("PK_EntityDamage | Failed to find BaseVehicleControllerComponent", LogLevel.ERROR);
			return;
		}
		
		vehicleController.ForceStartEngine();
	}

	protected void SetInitialLightStates()
	{

		BaseLightManagerComponent lightManager = BaseLightManagerComponent.Cast(GetOwner().FindComponent(BaseLightManagerComponent));
		if (!lightManager) {
			Print("PK_EntityDamage | Failed to find BaseLightManagerComponent", LogLevel.ERROR);
			return;
		}
		
		foreach (PK_LightData lightData : m_aLightsData)
		{
			if(!lightData.m_bLightEnabled)
				continue;
			lightManager.SetLightsState(lightData.m_eLightType, true, lightData.m_iLightSide);
		}
	}
	
	protected void SetInitialHitZoneHealth()
	{
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(GetOwner().FindComponent(SCR_DamageManagerComponent));
		if (!damageManager) {
			Print("PK_EntityDamage | Failed to find SCR_DamageManagerComponent", LogLevel.ERROR);
			return;
		}
		
		foreach (PK_HitzoneData hitZoneData : m_aHitzonesData)
		{
			if(!hitZoneData.m_bHitZoneEnabled)
				continue;

			SCR_HitZone hitZone = GetPhysicalHitZone(hitZoneData.m_sTargetZone);
			if (!hitZone)
				continue;

			string targetZoneName = hitZoneData.m_sTargetZone;
			targetZoneName.ToUpper();

			if (hitZoneData.m_fHealthPercentage != 1.0)
				//Hitzones like wheel or lights need special treatment or it will just damage the first entry
				//Needs improvment to work for all types
				if(targetZoneName.Contains("WHEEL"))
				{
					array<HitZone> physicalHitZones = {};
					damageManager.GetPhysicalHitZones(physicalHitZones);
					SCR_WheelHitZone wheelHitZone;
					foreach (HitZone physicalHitZone : physicalHitZones)
					{
						wheelHitZone = SCR_WheelHitZone.Cast(physicalHitZone);
				        if (wheelHitZone)
				        {
							if(hitZoneData.m_iIndexId == wheelHitZone.GetWheelIndex())
								physicalHitZone.SetHealthScaled(hitZoneData.m_fHealthPercentage);
				        }
					}

				}
				else
				{
					hitZone.SetHealthScaled(hitZoneData.m_fHealthPercentage);
				}
		}
	}
	
	protected void DebugPrintAllHitZones()
	{
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(GetOwner().FindComponent(SCR_DamageManagerComponent));
		if (!damageManager) {
			Print("PK_EntityDamage | Failed to find SCR_DamageManagerComponent", LogLevel.ERROR);
			return;
		}
		
		array<HitZone> physicalHitZones = {};
		damageManager.GetPhysicalHitZones(physicalHitZones);
		
		SCR_WheelHitZone wheelHitZone;
		string ownerName = GetOwner().GetName();
		foreach (HitZone physicalHitZone : physicalHitZones)
		{
			wheelHitZone = SCR_WheelHitZone.Cast(physicalHitZone);
	        if (wheelHitZone)
				Print("PK_Hitzones - Name: " + ownerName + " - Hitzone Name: " + physicalHitZone.GetName() + " - Index Id: " + wheelHitZone.GetWheelIndex());
			else
				Print("PK_Hitzones - Name: " + ownerName + " - Hitzone Name: " + physicalHitZone.GetName());
		}
	}

	

	protected SCR_HitZone GetPhysicalHitZone(string m_sTargetZone)
	{
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(GetOwner().FindComponent(SCR_DamageManagerComponent));
		if (!damageManager) {
			Print("PK_EntityDamage | Failed to find SCR_DamageManagerComponent", LogLevel.ERROR);
			return null;
		}
		
		HitZone hitZone;
		if (m_sTargetZone != "")
			hitZone = damageManager.GetHitZoneByName(m_sTargetZone);
		else
			hitZone = damageManager.GetDefaultHitZone();
		if (!hitZone) {
			Print("PK_EntityDamage | Failed to find '" + m_sTargetZone + "' Hitzone - flag will not work.", LogLevel.ERROR);
			return null;
		}
		return SCR_HitZone.Cast(hitZone);
	}

	
}


[BaseContainerProps()]
class PK_LightData
{
	[Attribute(defvalue: SCR_Enum.GetDefault(ELightType.Head), uiwidget: UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(ELightType))]
	ELightType m_eLightType;

	//! Side of turn signals
	[Attribute("-1", uiwidget: UIWidgets.ComboBox, enums: { ParamEnum("Either", "-1"), ParamEnum("Left", "0"), ParamEnum("Right", "1")})]
	int m_iLightSide;
	
	[Attribute("1", UIWidgets.CheckBox, "Enable this specfic light.")]
	bool m_bLightEnabled;
}


[BaseContainerProps()]
class PK_HitzoneData
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.Auto, desc: "Name of target hitzone. Enable Debug print all hitzones to get a list of all hitzones. Keep empty for default, will normally kill the entity", category: "Damage Manager")]
	string m_sTargetZone;
	
	[Attribute(defvalue: "", uiwidget: UIWidgets.Auto, desc: "The hitzone index Id. Only requrired when there are more than one of the type like Wheels or lights", category: "Damage Manager")]
	int m_iIndexId;
	
	[Attribute("100", UIWidgets.Slider, params: "0 100 1", desc: "The initial health percentage of the hitzone should be.", category: "Damage Manager")]
	float m_fHealthPercentage;
	
	[Attribute("1", UIWidgets.CheckBox, "Enable this specfic hitzone.", category: "Damage Manager")]
	bool m_bHitZoneEnabled;
}