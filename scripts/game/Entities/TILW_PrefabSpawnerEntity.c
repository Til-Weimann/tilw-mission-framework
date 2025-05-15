[EntityEditorProps(category: "GameScripted/", description: "Entity which spawns prefabs at runtime, with some extra options.")]
class TILW_PrefabSpawnerEntityClass: GenericEntityClass
{
}

class TILW_PrefabSpawnerEntity : GenericEntity
{
	
	[Attribute("", UIWidgets.ResourceNamePicker, "Prefab to spawn", "et", category: "Spawning")]
	protected ResourceName m_prefab;
	
	[Attribute("", uiwidget: UIWidgets.Auto, desc: "If defined, the entity is only spawned when this flag is set.\nYou can define more complex conditions using meta flags.", category: "Spawning")]
	protected string m_conditionFlag;
	
	[Attribute("", UIWidgets.Auto, desc: "Optionally, define when the prefab spawns (seconds after the condition is first met). Multiple entries will result in multiple spawns.", category: "Spawning")]
	protected ref array<float> m_spawnTimings;
	
	[Attribute("", uiwidget: UIWidgets.Auto, desc: "If you need to reference the spawned entities later, you can give them UNIQUE names - otherwise, leave empty.", category: "Spawning")]
	protected ref array<string> m_setEntityNames;
	
	[Attribute("0", UIWidgets.Auto, desc: "Already allow spawning before the briefing has ended.", category: "Spawning")]
	protected bool m_pregameSpawn;
	
	
	[Attribute("", UIWidgets.Auto, desc: "If the prefab is an AI Group, add existing waypoints to the spawned AI group by name", category: "Configuration")]
	protected ref array<string> m_waypointNames;
	
	[Attribute("", UIWidgets.Object, desc: "If the prefab is a vehicle, define its crew - you may drag existing configs into here.", category: "Configuration")]
	protected ref TILW_CrewConfig m_crewConfig;
	
	void TILW_PrefabSpawnerEntity(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!GetGame().InPlayMode() && !Replication.IsServer())
			return;
		GetGame().GetCallqueue().CallLater(Init, 1000, false);
	}
	
	protected void Init()
	{
		if (StateAllowed() && FlagAllowed())
		{
			InitSpawn();
			return;
		}
		
		// Insert Listeners
		if (!m_pregameSpawn)
		{
			PS_GameModeCoop gm = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			gm.GetOnGameStateChange().Insert(StateChange);
		}
		if (m_conditionFlag != "")
		{
			TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
			fw.GetOnFlagChanged().Insert(FlagChange);
		}
	}
	
	protected void FlagChange(string flag, bool value)
	{
		if (flag == m_conditionFlag && value)
			CheckConditions();
	}
	
	protected void StateChange(SCR_EGameModeState state)
	{
		if (state == SCR_EGameModeState.GAME)
			CheckConditions();
	}
	
	protected void CheckConditions()
	{
		if (!StateAllowed() || !FlagAllowed())
			return;
		
		// Remove Listeners
		if (!m_pregameSpawn)
		{
			PS_GameModeCoop gm = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			gm.GetOnGameStateChange().Remove(StateChange);
		}
		if (m_conditionFlag != "")
		{
			TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
			fw.GetOnFlagChanged().Remove(FlagChange);
		}
		
		InitSpawn();
	}
	
	
	protected bool StateAllowed()
	{
		if (m_pregameSpawn)
			return true;
		
		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		return (gm && gm.GetState() == SCR_EGameModeState.GAME);
	}
	
	protected bool FlagAllowed()
	{
		if (m_conditionFlag == "")
			return true;
		
		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		return (fw && fw.IsMissionFlag(m_conditionFlag));
	}
	
	protected void InitSpawn()
	{
		if (m_spawnTimings.IsEmpty())
			GetGame().GetCallqueue().Call(SpawnPrefab);
		else
			foreach (float delay : m_spawnTimings)
				GetGame().GetCallqueue().CallLater(SpawnPrefab, delay * 1000, false);
	}
	
	
	protected void SpawnPrefab()
	{
		if (m_prefab == "")
			return;
		
		// Spawn
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		vector mat[4];
		GetWorldTransform(mat);
		spawnParams.Transform = mat;
		IEntity spawnedEntity = GetGame().SpawnEntityPrefab(Resource.Load(m_prefab), GetGame().GetWorld(), spawnParams);
		
		if (!spawnedEntity)
			return;
		
		// Set name
		if (!m_setEntityNames.IsEmpty())
		{
			spawnedEntity.SetName(m_setEntityNames[0]);
			m_setEntityNames.RemoveOrdered(0);
		}
		
		// If it is group, assign waypoints
		AIGroup g = AIGroup.Cast(spawnedEntity);
		if (g && !m_waypointNames.IsEmpty())
			TILW_CrewGroup.AssignWaypoints(g, m_waypointNames, false);
		
		// If it has compartments, assign crew
		SCR_BaseCompartmentManagerComponent cm = SCR_BaseCompartmentManagerComponent.Cast(spawnedEntity.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (m_crewConfig && cm)
		{
			GetGame().GetCallqueue().Call(m_crewConfig.SpawnCrew, cm);
			return;
		}
	}
	
	
	
	
#ifdef WORKBENCH
	
	protected IEntity m_previewEntity;
	
	override void _WB_OnInit(inout vector mat[4], IEntitySource src)
	{
		UpdateEntityPreview();
	}
	
	void UpdateEntityPreview()
	{
		if (m_previewEntity)
			SCR_EntityHelper.DeleteEntityAndChildren(m_previewEntity);
		
		Resource resource = Resource.Load(m_prefab);
		if (!resource || !resource.IsValid())
			return;
		
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		GetWorldTransform(spawnParams.Transform);
		m_previewEntity = GetGame().SpawnEntityPrefab(resource, GetWorld(), spawnParams);
	}

	override bool _WB_OnKeyChanged(BaseContainer src, string key, BaseContainerList ownerContainers, IEntity parent)
	{
		if (key == "coords" || key == "m_prefab")
		{
			UpdateEntityPreview();
		}
		return super._WB_OnKeyChanged(src, key, ownerContainers, parent);
	}
	
	void ~TILW_PrefabSpawnerEntity()
	{
		SCR_EntityHelper.DeleteEntityAndChildren(m_previewEntity);
	}
	
#endif
	
}