//! TILW_MissionEvent is a basic mission event. When its expression becomes true, it executes all instructions, then deactivates itself.
[BaseContainerProps(), BaseContainerCustomTitleField("m_name")]
class TILW_MissionEvent
{
	bool m_alreadyOccurred = false;
	
	[Attribute("Mission Event", UIWidgets.Auto, desc: "For readability and print debugging, this does not usually affect the mission.\nIt can also be used to reference events in reactivation instructions.")];
	string m_name;
	
	[Attribute("", UIWidgets.Object, desc: "Mission instructions that are executed when the event occurs")]
	ref array<ref TILW_BaseInstruction> m_instructions;
	
	[Attribute("", UIWidgets.Object, desc: "Boolean expression defining under what conditions this event should occur. \nConsists of (true when): Literal (flag), Conjunction (ALL operands), Disjunction (ANY operand), Minjunction (MIN operands), Maxjunction (MAX operands).")]
	ref TILW_BaseTerm m_condition;
	
	[Attribute("0", UIWidgets.Auto, desc: "Can this event already run before the briefing has ended (as opposed to just during the game phase)? \nIt is highly recommended to only use this with random (or no) flags in the condition, since many common flags will not be initialized yet.")];
	bool m_pregameEvent;
	
	void EvalExpression(bool ignoreCondition = false, bool ignoreOccurred = false)
	{
		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gm.GetState() != SCR_EGameModeState.GAME && !m_pregameEvent)
			return;
		
		if (m_alreadyOccurred && !ignoreOccurred)
			return;
		if (!m_condition.Eval() && !ignoreCondition)
			return;
		m_alreadyOccurred = true;
		
		Print("TILWMF | Running mission event: " + m_name, LogLevel.NORMAL);
		foreach (TILW_BaseInstruction instruction : m_instructions)
			GetGame().GetCallqueue().CallLater(instruction.Execute, instruction.m_executionDelay * 1000, false, instruction);
	}
	
	void SetName(string name)
	{
		m_name = name;
	}
	
	void SetInstructions(array<ref TILW_BaseInstruction> instructions)
	{
		m_instructions = instructions;
	}

	void SetCondition(TILW_BaseTerm condition)
	{
		m_condition = condition;
	}
}


//! TILW_BaseInstruction, only provides shared functionality. Instructions are executed by mission events.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Base Instruction (NOT USEFUL)")]
class TILW_BaseInstruction
{
	[Attribute("5", UIWidgets.Auto, desc: "After how many seconds to execute the instruction", params: "0 inf 0.1")]
	float m_executionDelay;
	
	void Execute();
	
	void SetExecutionDelay(float delay)
	{
		m_executionDelay = delay;
	}
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
		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		Faction faction = GetGame().GetFactionManager().GetFactionByKey(m_factionKey);
		
		if (!fw.m_suppressGameEnd)
		{
			int fIndex = GetGame().GetFactionManager().GetFactionIndex(faction);
			gm.EndGameMode(SCR_GameModeEndData.CreateSimple(m_gameOverType, -1, fIndex));
		}
		else
		{
			SCR_GameOverScreenManagerComponent gosmc = SCR_GameOverScreenManagerComponent.Cast(gm.FindComponent(SCR_GameOverScreenManagerComponent));
			string factionName = faction.GetFactionName();
			string endTitle = gosmc.GetGameOverConfig().GetGameOverInfo(m_gameOverType).GetOptionalParams().m_sTitle;
			fw.ShowGlobalHint("MISSION END", "End conditions have been met:\n" + factionName + " " + endTitle, 10, null);
		}
	}
	
	void SetKey(string key)
	{
		m_factionKey = key;
	}
	
	void SetGameOverType(EGameOverTypes gameOverType)
	{
		m_gameOverType = gameOverType;
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
		if (!e)
			return;
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		vector mat[4];
		e.GetWorldTransform(mat);
		spawnParams.Transform = mat;
		m_spawnedEntity = GetGame().SpawnEntityPrefab(Resource.Load(m_prefab), GetGame().GetWorld(), spawnParams);
		if (m_spawnedEntity && m_setEntityName != "")
			m_spawnedEntity.SetName(m_setEntityName);
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
		if (!m_spawnedEntity)
			return;
		AIGroup group = AIGroup.Cast(m_spawnedEntity);
		if (!group)
			return;
		
		TILW_CrewGroup.AssignWaypoints(group, m_waypointNames);
	}
}

//! TILW_SpawnVehicleInstruction spawns an vehicle, crewed by default passengers. 
[BaseContainerProps(), BaseContainerCustomStringTitleField("Spawn Crewed Vehicle")]
class TILW_SpawnVehicleInstruction : TILW_SpawnPrefabInstruction
{
	[Attribute("", UIWidgets.Object, desc: "Defines the vehicles crew - you may drag existing configs into here.", category: "Crew")]
	protected ref TILW_CrewConfig m_crewConfig;
	
	
	[Attribute("1", UIWidgets.Auto, desc: "DEPRECATED - Spawn Driver", category: "Deprecated")]
	protected bool m_spawnPilot;
	[Attribute("1", UIWidgets.Auto, desc: "DEPRECATED - Spawn Gunner", category: "Deprecated")]
	protected bool m_spawnTurret;
	[Attribute("1", UIWidgets.Auto, desc: "DEPRECATED - Spawn Passengers", category: "Deprecated")]
	protected bool m_spawnCargo;
	
	[Attribute("", UIWidgets.ResourceAssignArray, desc: "DEPRECATED - If defined, fill seats with these characters instead of the vehicles default characters. \nList of character prefabs, empty resources will become empty seats. \nYou can check the vehicles CompartmentManagerComponent for the primary compartments slot order, secondary compartsments (e. g. BTR gunner) come afterwards.", category: "Deprecated", params: "et")]
	protected ref array<ResourceName> m_customCrew;
	
	[Attribute("", UIWidgets.Auto, desc: "DEPRECATED - Names of existing waypoints to be assinged after waypoint delay", category: "Deprecated")]
	protected ref array<string> m_waypointNames;
	[Attribute("5", UIWidgets.Auto, desc: "DEPRECATED - After how many additional seconds to assign the waypoints", category: "Deprecated", params: "0 inf 0.1")]
	protected float m_waypointDelay;
	
	[Attribute("0", UIWidgets.Auto, desc: "DEPRECATED - Put all gunners into a separate group, so that they remain idle (and can engage targets), while only driver + passengers follow the waypoint.", category: "Deprecated")]
	protected bool m_idleGroup;
	
	override void Execute()
	{
		super.Execute();
		GetGame().GetCallqueue().Call(AddCrew);
	}
	
	protected void AddCrew()
	{
		if (!m_spawnedEntity)
			return;
		SCR_BaseCompartmentManagerComponent cm = SCR_BaseCompartmentManagerComponent.Cast(m_spawnedEntity.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!cm)
			return;
		
		if (m_crewConfig)
		{
			GetGame().GetCallqueue().Call(m_crewConfig.SpawnCrew, cm);
			return;
		}
		
		array<BaseCompartmentSlot> slots = {};
		cm.GetCompartments(slots);
		
		array<ECompartmentType> cTypes = {};
		if (m_spawnPilot)
			cTypes.Insert(ECompartmentType.PILOT);
		if (m_spawnTurret)
			cTypes.Insert(ECompartmentType.TURRET);
		if (m_spawnCargo)
			cTypes.Insert(ECompartmentType.CARGO);
		
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
		if (!ge)
			return;
		AIGroup group = AIGroup.Cast(ge);
		if (!group)
			return;
		
		TILW_CrewGroup.AssignWaypoints(group, m_waypointNames, m_clearExisting, false);
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
			if (e)
				SCR_EntityHelper.DeleteEntityAndChildren(e);
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
		TILW_MissionFrameworkEntity.GetInstance().AdjustMissionFlag(m_flagName, true);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Clear Flag")]
class TILW_ClearFlagInstruction : TILW_BaseInstruction
{
	[Attribute("", UIWidgets.Auto, desc: "Name of the flag which should be cleared.")]
	protected string m_flagName;
	
	override void Execute()
	{
		TILW_MissionFrameworkEntity.GetInstance().AdjustMissionFlag(m_flagName, false);
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
		if (!pm)
			return;
		foreach (RplId id, PS_PlayableContainer pc : pm.GetPlayables())
		{
			PS_PlayableComponent pcomp = pc.GetPlayableComponent();
			
			if (!pc || pm.GetPlayerByPlayable(pcomp.GetRplId()) != -1)
				continue;
			
			IEntity e = pcomp.GetOwner();
			if (!m_factionKeys.IsEmpty())
			{
				SCR_ChimeraCharacter cc = SCR_ChimeraCharacter.Cast(e);
				if (!cc || !cc.m_pFactionComponent)
					continue;
				Faction f = cc.m_pFactionComponent.GetAffiliatedFaction();
				if (!f || !m_factionKeys.Contains(f.GetFactionKey()))
					continue;
			}
			
			SCR_EntityHelper.DeleteEntityAndChildren(e);
		}
	}

}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Reactivate Events")]
class TILW_ReactivateEventsInstruction : TILW_BaseInstruction
{
	[Attribute("", UIWidgets.Auto, desc: "Names of the mission events to be reactivated - after execution, these events will be allowed to run another time.")]
	protected ref array<string> m_eventNames;
	
	override void Execute()
	{
		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		if (!fw || !m_eventNames)
			return;
		foreach (TILW_MissionEvent me : fw.m_missionEvents)
			if (me.m_alreadyOccurred && m_eventNames.Contains(me.m_name))
				me.m_alreadyOccurred = false;
		GetGame().GetCallqueue().Call(fw.RecheckConditions);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Deactivate Events")]
class TILW_DeactivateEventsInstruction : TILW_BaseInstruction
{
	[Attribute("", UIWidgets.Auto, desc: "Names of the mission events to be deactivated - after execution, these events will not be able to run anymore.")]
	protected ref array<string> m_eventNames;
	
	override void Execute()
	{
		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		if (!fw || !m_eventNames)
			return;
		foreach (TILW_MissionEvent me : fw.m_missionEvents)
			if (!me.m_alreadyOccurred && m_eventNames.Contains(me.m_name))
				me.m_alreadyOccurred = true;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Edit Respawn Tickets")]
class TILW_EditRespawnTicketsInstruction : TILW_BaseInstruction
{
	[Attribute("", UIWidgets.Auto, desc: "Key of the affected faction.")]
	protected string m_factionKey;
	
	[Attribute("0", UIWidgets.ComboBox, "How to affect the factions ticket count.", enums: ParamEnumArray.FromEnum(TILW_EVariableOperation))]
	protected TILW_EVariableOperation m_operation;
	
	[Attribute("0", UIWidgets.Auto, desc: "By what amount to affect the factions ticket count.", params: "0 inf")]
	protected int m_amount;
	
	[Attribute("0", UIWidgets.Auto, desc: "If this operation made tickets available, try to respawn players who were previously sent into spectator.")]
	bool m_allowSpecRespawn;
	
	override void Execute()
	{
		BaseGameMode bgm = GetGame().GetGameMode();
		if (!bgm)
			return;
		PS_GameModeCoop gm = PS_GameModeCoop.Cast(bgm);
		int num = gm.TILW_GetFactionTicketCount(m_factionKey);
		switch (m_operation) {
			case TILW_EVariableOperation.SET:
				num = m_amount;
				break;
			case TILW_EVariableOperation.ADD:
				num += m_amount;
				break;
			case TILW_EVariableOperation.SUB:
				num -= m_amount;
				break;
			case TILW_EVariableOperation.MUL:
				num *= m_amount;
				break;
			case TILW_EVariableOperation.DIV:
				num /= m_amount;
				break;
			default:
				return;
		}
		gm.TILW_SetFactionTicketCount(m_factionKey, num);
		
		if (m_allowSpecRespawn) {
			PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
			array<PS_PlayableContainer> playableContainers = playableManager.GetPlayablesSorted();
			foreach (PS_PlayableContainer container : playableContainers)
			{
				PS_PlayableComponent pcomp = container.GetPlayableComponent();
				SCR_CharacterDamageManagerComponent damageManager = pcomp.GetCharacterDamageManagerComponent();
				EDamageState damageState = damageManager.GetState();
				if (damageState == EDamageState.DESTROYED)
				{
					int playerId = playableManager.GetPlayerByPlayableRemembered(pcomp.GetRplId());
					if (playerId == -1)
						continue;
					gm.TryRespawn(pcomp.GetRplId(), playerId);
				}
			}
		}
	}
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_factionKey")]
class TILW_FactionVisibility
{
	[Attribute("", UIWidgets.Auto, desc: "Faction key to alter visibility of.")]
	string m_factionKey;

	[Attribute("", UIWidgets.Auto, desc: "Whether or not the map element should be visible to this faction.")]
	bool m_visible;
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Edit map marker/description")]
class TILW_EditMapItemInstruction : TILW_BaseInstruction
{
	[Attribute("", UIWidgets.Auto, desc: "Names of the affected map marker or mission description entities.")]
	protected ref array<string> m_itemNames;
	
	[Attribute("", UIWidgets.Auto, "Which factions the marker should be set to visible or not visible for.")]
	protected ref array<ref TILW_FactionVisibility> m_factionVisibility;
	
	[Attribute("0", UIWidgets.Auto, desc: "Whether or not the marker should be visible to the empty faction.")]
	protected bool m_visibleForEmpty;
	
	// If required, this could be extended with other properties like title, though only some are shared between marker and description
	
	override void Execute()
	{
		foreach (string itemName : m_itemNames)
		{
			IEntity itemEntity = GetGame().GetWorld().FindEntityByName(itemName);
			if (!itemEntity)
				continue;
			PS_ManualMarker c1 = PS_ManualMarker.Cast(itemEntity);
			PS_MissionDescription c2 = PS_MissionDescription.Cast(itemEntity);
			if (c1)
				c1.SetVisibleForEmptyFaction(m_visibleForEmpty);
			if (c2)
				c2.SetVisibleForEmptyFaction(m_visibleForEmpty);
			SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			foreach (TILW_FactionVisibility fv : m_factionVisibility)
			{
				Faction f = factionManager.GetFactionByKey(fv.m_factionKey);
				if (c1)
					c1.SetVisibleForFaction(f, fv.m_visible);
				if (c2)
					c2.SetVisibleForFaction(f, fv.m_visible);
			}
		}
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Run Mission Events")]
class TILW_RunEventsInstruction : TILW_BaseInstruction
{
	[Attribute("", UIWidgets.Auto, desc: "Names of mission events to run - after the delay, these events will be executed, regardless of their conditions.")]
	protected ref array<string> m_eventNames;
	
	[Attribute("0", UIWidgets.Auto, desc: "If true, even run events that already occured previously.")]
	protected bool m_allowRerun;
	
	override void Execute()
	{
		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		if (!fw || !m_eventNames)
			return;
		foreach (TILW_MissionEvent me : fw.m_missionEvents)
			if (m_eventNames.Contains(me.m_name))
				me.EvalExpression(true, m_allowRerun);
	}
}