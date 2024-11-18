//! TILW_MissionEvent is a basic mission event. When its expression becomes true, it executes all instructions, then deactivates itself.
[BaseContainerProps(), BaseContainerCustomTitleField("m_name")]
class TILW_MissionEvent
{
	bool m_alreadyOccurred = false;
	
	[Attribute("Mission Event", UIWidgets.Auto, desc: "Just for readability and print debugging, this does not affect the mission")];
	string m_name;
	
	[Attribute("", UIWidgets.Object, desc: "Mission instructions that are executed when the event occurs")]
	ref array<ref TILW_BaseInstruction> m_instructions;
	
	[Attribute("", UIWidgets.Object, desc: "Boolean expression defining under what conditions this event should occur. \nConsists of (true when): Literal (flag), Conjunction (ALL operands), Disjunction (ANY operand), Minjunction (MIN operands), Maxjunction (MAX operands).")]
	ref TILW_BaseTerm m_condition;
	
	void EvalExpression()
	{
		if (m_alreadyOccurred || !m_condition.Eval()) return;
		m_alreadyOccurred = true;
		Print("TILWMF | Running mission event: " + m_name, LogLevel.NORMAL);
		foreach (TILW_BaseInstruction instruction : m_instructions) GetGame().GetCallqueue().CallLater(instruction.Execute, instruction.m_executionDelay * 1000, false, instruction);
	}
}




//! TILW_BaseInstruction, only provides shared functionality. Instructions are executed by mission events.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Base Instruction (NOT USEFUL)")]
class TILW_BaseInstruction
{
	[Attribute("5", UIWidgets.Auto, desc: "After how many seconds to execute the instruction", params: "0 inf 0.1")]
	float m_executionDelay;
	
	void Execute();
}


//! TILW_EndGameInstruction ends the mission with a certain faction as e. g. winner
[BaseContainerProps(), BaseContainerCustomStringTitleField("End Game")]
class TILW_EndGameInstruction: TILW_BaseInstruction
{
	// message, end delay, faction key, game over Type
	[Attribute(defvalue: SCR_Enum.GetDefault(EGameOverTypes.EDITOR_FACTION_VICTORY), uiwidget: UIWidgets.ComboBox, desc: "How to end the game after the expression turned true", enums: ParamEnumArray.FromEnum(EGameOverTypes))]
	protected EGameOverTypes m_gameOverType;
	
	[Attribute("", UIWidgets.Auto, desc: "Key of the faction (which e. g. won or lost)")]
	protected string m_factionKey;
	
	override void Execute()
	{
		if (TILW_MissionFrameworkEntity.GetInstance().m_suppressGameEnd) return;
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		Faction faction = GetGame().GetFactionManager().GetFactionByKey(m_factionKey);
		int fIndex = GetGame().GetFactionManager().GetFactionIndex(faction);
		gameMode.EndGameMode(SCR_GameModeEndData.CreateSimple(m_gameOverType, -1, fIndex));
	}
	
}


//! TILW_SendMessageInstruction displays a message, either to everyone or to certain factions
[BaseContainerProps(), BaseContainerCustomStringTitleField("Send Message")]
class TILW_SendMessageInstruction : TILW_BaseInstruction
{	
	[Attribute("", UIWidgets.Auto, desc: "Title of displayed message")]
	protected string m_messageTitle;
	
	[Attribute("", UIWidgets.EditBoxMultiline, desc: "Body of displayed message")]
	protected string m_messageBody;
	
	[Attribute("", UIWidgets.Auto, desc: "Factions that receive the message (if empty, everyone)")]
	protected ref array<string> m_factionKeys;
	
	override void Execute()
	{
		TILW_MissionFrameworkEntity.GetInstance().ShowGlobalHint(m_messageTitle, m_messageBody, 10, m_factionKeys);
	}
}


//! TILW_SpawnPrefabInstruction spawns a prefab at the location of an existing entity
[BaseContainerProps(), BaseContainerCustomStringTitleField("Spawn Prefab")]
class TILW_SpawnPrefabInstruction : TILW_BaseInstruction
{
	[Attribute("", UIWidgets.ResourceNamePicker, "Prefab to spawn", "et")]
	ResourceName m_prefab;
	
	[Attribute("", UIWidgets.Auto, desc: "Name of existing entity at which the prefab should be spawned")]
	protected string m_locationName;
	
	[Attribute("", UIWidgets.Auto, desc: "If you need to reference the spawned entity later, you can give it a UNIQUE name - otherwise, leave empty.")]
	protected string m_setEntityName;
	
	protected IEntity m_spawnedEntity;
	
	override void Execute()
	{
		IEntity e = GetGame().GetWorld().FindEntityByName(m_locationName);
		if (!e) return;
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		vector mat[4];
		e.GetWorldTransform(mat);
		spawnParams.Transform = mat;
		m_spawnedEntity = GetGame().SpawnEntityPrefab(m_prefab, true, GetGame().GetWorld(), spawnParams);
		if (m_spawnedEntity && m_setEntityName != "") m_spawnedEntity.SetName(m_setEntityName);
	}
	
}


//! TILW_SpawnPrefabInstruction spawns an AI group prefab at the location of an existing entity, then adds existing waypoints to it
[BaseContainerProps(), BaseContainerCustomStringTitleField("Spawn AI Group")]
class TILW_SpawnGroupInstruction : TILW_SpawnPrefabInstruction
{
	
	[Attribute("", UIWidgets.Auto, desc: "If defined, add existing waypoints to the spawned AI group by name")]
	protected ref array<string> m_waypointNames;
	
	override void Execute()
	{
		super.Execute();
		if (!m_spawnedEntity) return;
		SCR_AIGroup group = SCR_AIGroup.Cast(m_spawnedEntity);
		if (!group) return;
		
		foreach (string wpn : m_waypointNames)
		{
			IEntity e = GetGame().GetWorld().FindEntityByName(wpn);
			if (!e) continue;
			AIWaypoint wp = AIWaypoint.Cast(e);
			if (wp) group.AddWaypoint(wp);
		}
	}
}

//! TILW_SpawnVehicleInstruction spawns an vehicle, crewed by default passengers. 
[BaseContainerProps(), BaseContainerCustomStringTitleField("Spawn Crewed Vehicle")]
class TILW_SpawnVehicleInstruction : TILW_SpawnPrefabInstruction
{
	[Attribute("1", UIWidgets.Auto, desc: "Spawn Driver", category: "Default Crew")]
	protected bool m_spawnPilot;
	[Attribute("1", UIWidgets.Auto, desc: "Spawn Gunner", category: "Default Crew")]
	protected bool m_spawnTurret;
	[Attribute("1", UIWidgets.Auto, desc: "Spawn Passengers", category: "Default Crew")]
	protected bool m_spawnCargo;
	
	[Attribute("", UIWidgets.ResourceAssignArray, desc: "If defined, spawn the manually specified crew instead of the vehicles default characters, ignoring the checkboxes above. \nList of character prefabs, empty resources will become empty seats. \nCheck the vehicles CompartmentManagerComponent for the primary compartments slot order, secondary compartments (e. g. BTR gunner) come afterwards.", category: "Crew", params: "et")]
	protected ref array<ResourceName> m_customCrew;
	
	[Attribute("", UIWidgets.Auto, desc: "Names of existing waypoints to be assinged after waypoint delay", category: "Waypoints")]
	protected ref array<string> m_waypointNames;
	[Attribute("5", UIWidgets.Auto, desc: "After how many additional seconds to assign the waypoints", category: "Waypoints", params: "0 inf 0.1")]
	protected float m_waypointDelay;
	
	[Attribute("0", UIWidgets.Auto, desc: "Put all gunners into a separate group, so that they remain idle (and can engage targets), while only driver + passengers follow the waypoint.", category: "Other")]
	protected bool m_idleGroup;
	
	override void Execute()
	{
		super.Execute();
		GetGame().GetCallqueue().Call(AddCrew);
	}
	
	protected void AddCrew()
	{
		if (!m_spawnedEntity) return;
		Managed m = m_spawnedEntity.FindComponent(SCR_BaseCompartmentManagerComponent);
		if (!m) return;
		SCR_BaseCompartmentManagerComponent cm = SCR_BaseCompartmentManagerComponent.Cast(m);
		
		array<BaseCompartmentSlot> slots = {};
		cm.GetCompartments(slots);
		
		array<ECompartmentType> cTypes = {};
		if (m_spawnPilot) cTypes.Insert(ECompartmentType.PILOT);
		if (m_spawnTurret) cTypes.Insert(ECompartmentType.TURRET);
		if (m_spawnCargo) cTypes.Insert(ECompartmentType.CARGO);
		
		GetGame().GetCallqueue().Call(TILW_VehicleCrewComponent.SpawnCrew, slots, cTypes, m_customCrew, null, null, m_waypointNames, m_waypointDelay, false, m_idleGroup);
	}
}


//! TILW_AssignWaypointsInstruction adds existing waypoints to an existing AI group, both referenced by name
[BaseContainerProps(), BaseContainerCustomStringTitleField("Assign Waypoints")]
class TILW_AssignWaypointsInstruction : TILW_BaseInstruction
{
	[Attribute("", UIWidgets.Auto, desc: "Name of existing group to assign waypoints to")]
	protected string m_groupName;
	
	[Attribute("", UIWidgets.Auto, desc: "If defined, add existing waypoints to an existing AI group")]
	protected ref array<string> m_waypointNames;
	
	[Attribute("0", UIWidgets.Auto, desc: "Also remove any existing waypoints from the AI group, leaving only the new ones.")]
	protected bool m_clearExisting;
	
	override void Execute()
	{
		IEntity ge = GetGame().GetWorld().FindEntityByName(m_groupName);
		if (!ge) return;
		SCR_AIGroup group = SCR_AIGroup.Cast(ge);
		if (!group) return;
		
		if (m_clearExisting)
		{
			array<AIWaypoint> wps = {};
			group.GetWaypoints(wps);
			foreach (AIWaypoint wp: wps) group.RemoveWaypoint(wp);
		}
		
		foreach (string wpn : m_waypointNames)
		{
			IEntity e = GetGame().GetWorld().FindEntityByName(wpn);
			if (!e) continue;
			AIWaypoint wp = AIWaypoint.Cast(e);
			if (wp) group.AddWaypoint(wp);
		}
	}
}


//! TILW_DeleteEntitiesInstruction deletes existing entities (and their children) by name
[BaseContainerProps(), BaseContainerCustomStringTitleField("Delete Entities")]
class TILW_DeleteEntitiesInstruction : TILW_BaseInstruction
{
	[Attribute("", UIWidgets.Auto, desc: "Names of entities to delete")]
	protected ref array<string> m_entityNames;
	
	override void Execute()
	{
		foreach (string name : m_entityNames)
		{
			IEntity e = GetGame().GetWorld().FindEntityByName(name);
			if (e) SCR_EntityHelper.DeleteEntityAndChildren(e);
		}
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Set Flag")]
class TILW_SetFlagInstruction : TILW_BaseInstruction
{
	[Attribute("", UIWidgets.Auto, desc: "Name of the flag which should be set.")]
	protected string m_flagName;
	
	override void Execute()
	{
		TILW_MissionFrameworkEntity.GetInstance().SetMissionFlag(m_flagName);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Clear Flag")]
class TILW_ClearFlagInstruction : TILW_BaseInstruction
{
	[Attribute("", UIWidgets.Auto, desc: "Name of the flag which should be cleared.")]
	protected string m_flagName;
	
	override void Execute()
	{
		TILW_MissionFrameworkEntity.GetInstance().ClearMissionFlag(m_flagName);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Delete Open Slots")]
class TILW_DeleteOpenSlotsInstruction : TILW_BaseInstruction
{
	[Attribute("", UIWidgets.Auto, desc: "Limit slot removal only to certain factions (if array is empty, all factions)")]
	protected ref array<string> m_factionKeys;
	
	override void Execute()
	{
		PS_PlayableManager pm = PS_PlayableManager.GetInstance();
		if (!pm) return;
		foreach (RplId id, PS_PlayableComponent pc : pm.GetPlayables())
		{
			if (!pc || pm.GetPlayerByPlayable(pc.GetId()) != -1) continue;
			
			IEntity e = pc.GetOwner();
			if (!m_factionKeys.IsEmpty()) {
				SCR_ChimeraCharacter cc = SCR_ChimeraCharacter.Cast(e);
				if (!cc || !cc.m_pFactionComponent) continue;
				Faction f = cc.m_pFactionComponent.GetAffiliatedFaction();
				if (!f || !m_factionKeys.Contains(f.GetFactionKey())) continue;
			}
			
			SCR_EntityHelper.DeleteEntityAndChildren(e);
		}
	}
}