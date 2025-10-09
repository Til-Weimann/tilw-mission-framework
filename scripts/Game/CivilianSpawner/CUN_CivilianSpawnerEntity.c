[EntityEditorProps(category: "GameScripted/", description: "Entity which randomly spawns and controls civilian groups.")]
class CUN_CivilianSpawnerEntityClass: GenericEntityClass
{
}

//This spawns civilians (or any other ai groups so could be used for random patrols) within a specified area. It also allows you to trigger the civilians spawned to flee when triggered.

class CUN_CivilianSpawnerEntity : GenericEntity
{
//Variables
//------------------------------------------------------------------------------------------------------------------------------------------------------------
	//Civilian Spawner
	
	[Attribute(uiwidget: UIWidgets.Auto, desc: "Group prefabs to spawn.", category: "Civilian Spawner")]
	protected ref array<ResourceName> m_aigroupPrefabs;
	
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Type of waypoint to use when wandering around", category: "Civilian Spawner")]
	protected ResourceName m_waypointPrefab;
	
	[Attribute(UIWidgets.Auto, desc: "Number of Groups to spawn.", params: "1 999 1", category: "Civilian Spawner", defvalue: "1")]
	protected int m_numberOfGroupsToSpawn;
	
	[Attribute("1", UIWidgets.Auto, desc: "Check this if you want to use buildings as civilian destinations. \nNOTE what reforger considers a building can be a little strange at times.", category: "Civilian Spawner")]
	protected bool m_useBuildingsAsDestinations;
	
	[Attribute(uiwidget: UIWidgets.Slider, desc: "Radius of search area for valid waypoint and spawn locations.", defvalue: "20", category: "Civilian Spawner", params: "0, 1000, 1")]
	protected float m_searchRadius;
	
	[Attribute(UIWidgets.Auto, desc: "Minimum delay before changing waypoint location in seconds.", defvalue: "10", category: "Civilian Spawner", params: "10,999,1")]
	protected int m_minWaypointChangeDelay;
	
	[Attribute(UIWidgets.Auto, desc: "Maximum delay before changing waypoint location in seconds.", defvalue: "20", category: "Civilian Spawner", params: "10,999,1")]
	protected int m_maxWaypointChangeDelay;
	
	//TilW FrameWork
	[Attribute("", uiwidget: UIWidgets.Auto, desc: "If defined, the spawned civilians will flee to Civilian Flee Entities if any are in the area. \nOtherwise this will do nothing.", category: "TilW Framework")]
	protected string m_fleeConditionFlag;
	
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Type of waypoint to use when fleeing", category: "TilW Framework")]
	protected ResourceName m_fleeWaypointPrefab;
	
	[Attribute("0", UIWidgets.Auto, desc: "If possible the civilians will try flee to their own individual hiding spot. \nTo work properly you need the same or more flee entites as civilians", category: "TilW Framework")]
	protected bool m_tryToFleeToSeperatePoints;
	
	[Attribute("", uiwidget: UIWidgets.Auto, desc: "If defined, the entities are only spawned when this flag is set.\nYou can define more complex conditions using meta flags.", category: "TilW Framework")]
	protected string m_spawnConditionFlag;
	
	[Attribute("0", UIWidgets.Auto, desc: "Already allow spawning before the briefing has ended.", category: "TilW Framework")]
	protected bool m_pregameSpawn;
	
	//Debug
	
	[Attribute("1", UIWidgets.Auto, desc: "Shows the search area in the editor.", category: "Debug")]
	protected bool m_showRadius;
	
	protected ref array<AIGroup> m_groups = new array<AIGroup>();
	protected ref array<AIWaypoint> m_waypoints = new array<AIWaypoint>();
	protected ref array<AIWaypoint> m_fleeWaypoints = new array<AIWaypoint>();
	protected ref array<IEntity> m_destinations = new array<IEntity>();
	protected ref array<IEntity> m_spawnLocations = new array<IEntity>();
	protected ref array<IEntity> m_fleeLocations = new array<IEntity>();
	protected ref array<int> m_timers = new array<int>();
	protected IEntity m_owner;
	
	void CUN_CivilianPopulatorEntity(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT);
	}
	
//Init
//------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void EOnActivate(IEntity owner)
	{
		
		if (!GetGame().InPlayMode() && !Replication.IsServer())
			return;
		GetGame().GetCallqueue().CallLater(Init, 1000, false);
		m_owner = owner;
	}
	
	protected void Init()
	{
		
		if (StateAllowed() && FlagAllowed())
		{	
			Start();
			return;		
		}
		
		if (!m_pregameSpawn)
		{
			PS_GameModeCoop gm = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			gm.GetOnGameStateChange().Insert(StateChange);
		}
		if (m_spawnConditionFlag != "")
		{
			TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
			fw.GetOnFlagChanged().Insert(FlagChange);
		}
	}
	
	protected void Start()
	{
		
		if (m_fleeConditionFlag != "")
		{
			TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
			fw.GetOnFlagChanged().Insert(CheckFleeCondition);
		}
		
		SpawnAllWaypoints(m_owner);
		
		if (m_waypoints.IsEmpty())
			return;
		
		SpawnGroups();
		
		if (m_groups.IsEmpty())
			return;
		
		GetGame().GetCallqueue().CallLater(Tick, 1000, true);
		
	}
	
//Waypoint Spawning
//------------------------------------------------------------------------------------------------------------------------------------------------------------
	protected void SpawnAllWaypoints(IEntity owner)
	{
		if (m_waypointPrefab == "")
			return;
		
		GetGame().GetWorld().QueryEntitiesBySphere(owner.GetOrigin(), m_searchRadius, QueryEntities);
		
		if (m_destinations.IsEmpty())
			return;
		
		
		int destinationCount = m_destinations.Count();
		
		for (int i = 0; i < destinationCount; i++)
		{
			EntitySpawnParams spawnParams = new EntitySpawnParams();
			vector transform[4];
			m_destinations[i].GetWorldTransform(transform);
			spawnParams.Transform = transform;
			
			IEntity spawnedEntity = GetGame().SpawnEntityPrefab(Resource.Load(m_waypointPrefab), GetGame().GetWorld(), spawnParams);
			
			AIWaypoint waypoint = AIWaypoint.Cast(spawnedEntity);
			
			//If it isn't a waypoint we just spawned we need to early exit and delete whatever we just spawned.
			if (!waypoint)
			{
				SCR_EntityHelper.DeleteEntityAndChildren(spawnedEntity);
				return;
			}
			
			m_waypoints.Insert(waypoint);
		}
		
		//NOTE this function is duplicated as array referencing doesn't appear to work also failure to find flee locations does not need to early exit the script.
		if (!m_fleeLocations.IsEmpty())
		{
			int fleeLocationCount = m_fleeLocations.Count();
			
			for (int i = 0; i < fleeLocationCount; i++)
			{
				EntitySpawnParams spawnParams = new EntitySpawnParams();
				vector transform[4];
				m_fleeLocations[i].GetWorldTransform(transform);
				spawnParams.Transform = transform;
				
				IEntity spawnedEntity = GetGame().SpawnEntityPrefab(Resource.Load(m_fleeWaypointPrefab), GetGame().GetWorld(), spawnParams);
				
				AIWaypoint waypoint = AIWaypoint.Cast(spawnedEntity);
				
				//If it isn't a waypoint we just spawned we need to early exit and delete whatever we just spawned.
				if (!waypoint)
				{
					SCR_EntityHelper.DeleteEntityAndChildren(spawnedEntity);
					return;
				}
				
				m_fleeWaypoints.Insert(waypoint);
			}
		}
		
		//tidy up.
		m_destinations.Clear();
		m_fleeLocations.Clear();
	}
	
	//This code executes on each entity within the search area.
	protected bool QueryEntities(IEntity entity)
	{
		//Is it a flee location?
		if (m_fleeConditionFlag != "")
		{
			CUN_CivilianFleeEntity fleeLocation = CUN_CivilianFleeEntity.Cast(entity);
			
			if (fleeLocation)
			{
				m_fleeLocations.Insert(entity);
				return true;
			}
		}
		
		//Is it a spawn location?
		CUN_CivilianSpawnEntity spawnLocation = CUN_CivilianSpawnEntity.Cast(entity);
		
		if (spawnLocation)
		{
			m_spawnLocations.Insert(entity);
			return true;
		}
		
		
		//Is it a building (if use buildings is set)
		if (m_useBuildingsAsDestinations)
		{
			SCR_DestructibleBuildingEntity building = SCR_DestructibleBuildingEntity.Cast(entity);
		
			if (building)
			{
				m_destinations.Insert(entity);
				return true;
			}
		}
		
		//Is it a regular destination?
		CUN_CivilianDestinationEntity destination = CUN_CivilianDestinationEntity.Cast(entity);
			
		if (destination)
		{
			m_destinations.Insert(entity);	
		}
			
		return true;
	}

//AI Group spawning
//------------------------------------------------------------------------------------------------------------------------------------------------------------

	protected void SpawnGroups()
	{
		for (int i = 0; i < m_numberOfGroupsToSpawn; i++)
		{
			SpawnGroup();
		}
	}
	
	protected void SpawnGroup()
	{
		if (!m_aigroupPrefabs) 
			return;
		
		if (m_aigroupPrefabs.IsEmpty())
			return;
		
		
		ResourceName prefab = m_aigroupPrefabs.GetRandomElement();
		
		if (prefab == "")
			return;
		
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		vector transform[4];
		
		
		if (!m_spawnLocations.IsEmpty())
			{
				m_spawnLocations.GetRandomElement().GetWorldTransform(transform);
			}
		else
			GetWorldTransform(transform);
		spawnParams.Transform = transform;
		
		IEntity spawnedEntity = GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), spawnParams);
		
		AIGroup group = AIGroup.Cast(spawnedEntity);
		
		//If the prefab isn't an AIGroup we need to early exit from the script and delete whatever we just spawned.
		if (!group)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(spawnedEntity);
			return;	
		}
		
		m_groups.Insert(group);
		m_timers.Insert(0);
	}
	
	protected void CheckFleeCondition(string flag, bool value)
	{
		if (flag == m_fleeConditionFlag && value)
			Flee();
	}

//Runtime Logic
//------------------------------------------------------------------------------------------------------------------------------------------------------------	

	protected void Tick()
	{
		for (int i = 0; i < m_groups.Count(); i++)
		{
			int timer = m_timers[i];
			timer--;
			
			if (timer <= 0)
			{
				SetRandomGroupWaypoint(m_groups[i]);
				m_timers[i] = Math.RandomInt(m_minWaypointChangeDelay, m_maxWaypointChangeDelay);
				if (i == 0)
			}
			else
				m_timers[i] = timer;
		}
	}
	
	protected void SetRandomGroupWaypoint(AIGroup group)
	{
		if (!group)
			return;
		
		array<AIWaypoint> waypoints = {};
		group.GetWaypoints(waypoints);
		if (waypoints.Count() > 0)
			foreach (AIWaypoint waypoint: waypoints)
				group.RemoveWaypoint(waypoint);
		
		AIWaypoint nextWaypoint = m_waypoints.GetRandomElement();
		group.AddWaypoint(nextWaypoint);
	}
	
	protected void SetRandomFleeWaypoint(AIGroup group)
	{
		if (!group)
			return;
		
		array<AIWaypoint> waypoints = {};
		group.GetWaypoints(waypoints);
		if (waypoints.Count() > 0)
			foreach (AIWaypoint waypoint: waypoints)
				group.RemoveWaypoint(waypoint);
		
		AIWaypoint fleeWaypoint = m_fleeWaypoints.GetRandomElement();
		
		if (m_tryToFleeToSeperatePoints && m_fleeWaypoints.Count() > 1)
			m_fleeWaypoints.RemoveItem(fleeWaypoint);
		
		group.AddWaypoint(fleeWaypoint);
	}
	
	protected void StateChange(SCR_EGameModeState state)
	{
		if (state == SCR_EGameModeState.GAME)
			CheckConditions();
	}
	
	protected void FlagChange(string flag, bool value)
	{
		if (flag == m_spawnConditionFlag && value)
			CheckConditions();
	}
	
	protected void Flee()
	{
		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		fw.GetOnFlagChanged().Remove(CheckFleeCondition);
		
		if (m_fleeWaypoints.IsEmpty())
			return;
		
		GetGame().GetCallqueue().Remove(Tick);
		
		foreach (AIGroup group: m_groups)
			SetRandomFleeWaypoint(group);
	}
	
	
//Check conditions
//------------------------------------------------------------------------------------------------------------------------------------------------------------
	
	protected bool StateAllowed()
	{
		if (m_pregameSpawn)
			return true;
		
		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		return (gm && gm.GetState() == SCR_EGameModeState.GAME);
	}
	
	protected bool FlagAllowed()
	{
		if (m_spawnConditionFlag == "")
			return true;
		
		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		return (fw && fw.IsMissionFlag(m_spawnConditionFlag));
	}
	
	protected void CheckConditions()
	{
		if (!StateAllowed() || !FlagAllowed())
			return;
		
		if (!m_pregameSpawn)
		{
			PS_GameModeCoop gm = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			gm.GetOnGameStateChange().Remove(StateChange);
		}
		if (m_spawnConditionFlag != "")
		{
			TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
			fw.GetOnFlagChanged().Remove(FlagChange);
		}
		
		Start();
	}

//Workbench tools
//------------------------------------------------------------------------------------------------------------------------------------------------------------
	
#ifdef WORKBENCH
	
	override int _WB_GetAfterWorldUpdateSpecs(IEntitySource src)
	{
		return EEntityFrameUpdateSpecs.CALL_ALWAYS;
	}
	
	override void _WB_AfterWorldUpdate(float timeSlice)
	{
		if (m_showRadius)
		{
			auto origin = GetOrigin();
			auto radiusShape = Shape.CreateSphere(COLOR_GREEN, ShapeFlags.WIREFRAME | ShapeFlags.ONCE, origin, m_searchRadius);
		}
		
		super._WB_AfterWorldUpdate(timeSlice);
	}
	
#endif
}