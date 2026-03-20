modded class PS_GameModeCoop : SCR_BaseGameMode
{
	// OPTIONS
	[Attribute("60", UIWidgets.Object, desc: "How many seconds should safe start last?", category: "Reforger Lobby", params: "0 inf 0")]
	int m_safeStartTime;
	
	override void TryRespawn(RplId playableId, int playerId)
	{
		super.TryRespawn(playableId, playerId);
		TILW_MissionFrameworkEntity mfe = TILW_MissionFrameworkEntity.GetInstance();
		if(mfe)
			mfe.PlayerUpdate(0, null);
	}

	void TILW_SetFactionTicketCount(string fkey, int num)
	{
		if(m_mFactionRespawnCount.Contains(fkey)) {
			m_mFactionRespawnCount.Get(fkey).m_iCount = num; // check if this works
		} else {
			PS_FactionRespawnCount frc = new PS_FactionRespawnCount();
			frc.m_sFactionKey = fkey;
			frc.m_iCount = num;
			m_mFactionRespawnCount.Insert(fkey, frc);
		}
	}

	int TILW_GetFactionTicketCount(string fkey)
	{
		PS_FactionRespawnCount frc = m_mFactionRespawnCount.Get(fkey);
		if(!frc)
			return 0;
		return frc.m_iCount;
	}

	// JIP SHIT
	[Attribute("2", UIWidgets.ComboBox, "What is the JIP TP behavior?", "", ParamEnumArray.FromEnum(EJIPState), category: "JIP")]
	EJIPState m_eJIPState;

	[Attribute("-1", UIWidgets.Auto, "If 0 or less disable JIP TP this many seconds into the mission.", category: "JIP")]
	int m_denyJIPTime;

	static const float MIN_JIP_TIME = 60;

	EJIPResponse CanJIP(SCR_PlayerController pc, out IEntity target)
	{
		if(m_eJIPState == EJIPState.Deny)
			return EJIPResponse.Disabled;
	
		float timeSeconds = GetElapsedTime() - GetGameStartElapsedTime();
		if(timeSeconds < MIN_JIP_TIME)
			return EJIPResponse.Min;
	
		if(m_denyJIPTime > 0 && m_denyJIPTime > timeSeconds)
			return EJIPResponse.Time;

		IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(pc.GetControlledEntity());
		if(vehicle)
			return EJIPResponse.InVehicle;

		SCR_AIGroup group = SCR_GroupsManagerComponent.GetInstance().GetPlayerGroup(pc.GetPlayerId());
		if(!group)
			return EJIPResponse.Group;
	
		array<IEntity> entites = {};

		IEntity leader = group.GetLeaderEntity();
		if(leader)
			entites.Insert(leader);
		
		array<AIAgent> agents = {};
		group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			IEntity _ent = agent.GetControlledEntity();
			if(_ent == leader)
				continue;
			
			entites.Insert(_ent);
		}
		
		foreach(IEntity entity : entites)
		{
			if(!IsValidJIPTarget(entity, pc))
				continue;
			
			target = entity;
			break;
		}
	
		if(!target)
			return EJIPResponse.Target;
	
		return EJIPResponse.Allowed;
	}
	
	bool IsValidJIPTarget(IEntity target, SCR_PlayerController pc)
	{
		if(!target || !EntityUtils.IsPlayer(target))
			return false;
		
		IEntity player = pc.GetControlledEntity();
		if(player == target)
			return false;

		IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(target);
		if(vehicle)
		{
			SCR_BaseCompartmentManagerComponent cm = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
			if(!cm)
				return false;
			
			bool hasFree = cm.HasFreeCompartmentOfTypes({ECompartmentType.CARGO, ECompartmentType.PILOT, ECompartmentType.TURRET});
			if(!hasFree)
				return false;
		}
	
		return true;
	}
	
	EJIPResponse TryJIP(SCR_PlayerController pc)
	{
		IEntity target;
		EJIPResponse response = CanJIP(pc, target);
		if(response != EJIPResponse.Allowed)
			return response;

		IEntity player = pc.GetControlledEntity();
		if(!player)
			return EJIPResponse.Target;
		
		IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(target);
		if(vehicle)
		{
			CompartmentAccessComponent access = CompartmentAccessComponent.Cast(target.FindComponent(CompartmentAccessComponent));
			if(!access)
				return EJIPResponse.Target;

			pc.GetInVehicle(vehicle);
			Print("TILWFW | Player JIPed: " + SCR_PlayerNamesFilterCache.GetInstance().GetPlayerDisplayName(pc.GetPlayerId()));
			
			return EJIPResponse.Sucess;
		}
		
		vector transform[4];
		target.GetWorldTransform(transform);
		
		vector forward = transform[2];
		forward[1] = 0;
		transform[3] = transform[3] - forward * 1.0;
		
		pc.Teleport(transform);
		Print("TILWFW | Player JIPed: " + SCR_PlayerNamesFilterCache.GetInstance().GetPlayerDisplayName(pc.GetPlayerId()));

		return EJIPResponse.Sucess;
	}
}

enum EJIPResponse
{
	Disabled,
	Min,
	Time,
	InVehicle,
	Group,
	Target,
	Allowed,
	Sucess
}

enum EJIPState
{
	Deny,
	Auto,
	Manual
}

[BaseContainerProps()]
class TILW_ManualJIPAvailable : SCR_AvailableActionCondition
{
	protected SCR_PlayerController pc;
	
	override bool IsAvailable(SCR_AvailableActionsConditionData data)
	{
		if(!pc)
		{
			pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			return false;
		}
		

		return pc.m_isJIPAvailable;
	}
}
