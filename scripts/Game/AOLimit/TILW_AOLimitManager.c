enum TILW_EVisibilityMode
{
	ALL = 0,
	FACTION = 1,
	NONE = 2
}

class GRAY_TESTClass : ScriptComponentClass
{
}

class GRAY_TEST : ScriptComponent
{
	protected override void OnPostInit(IEntity owner)
	{
		GetGame().GetCallqueue().CallLater(DeleteP, 10000);
	}
	
	void DeleteP()
	{
		Print("TILW AO | Delete");
		SCR_EntityHelper.DeleteEntityAndChildren(GetOwner());
	}
}

class TILW_AOLimitManagerClass : GenericEntityClass
{
}

class TILW_AOLimitManager : GenericEntity
{
	[Attribute("20", UIWidgets.Auto, "After how many seconds outside of AO players are killed", params: "0 inf 0", category: "Logic")]
	float m_killTimer;
	
	[Attribute("35", UIWidgets.Auto, "Kill timer if passenger in a vehicle", params: "0 inf 0", category: "Logic")]
	float m_vehicleKillTimer;
	
	protected const string PREFAB = "{42D6CF3CCE559B8C}Prefabs/Logic/AOLimit/TILW_AOLimitManager.et";
	protected const float CHECKFREQUENCY = 0.5;
	
	protected static TILW_AOLimitManager s_Instance;
	
	protected ref array<TILW_AOLimitComponent> m_aos = new array<TILW_AOLimitComponent>();
	
	protected float m_timeLeft = 0;
	protected float m_checkDelta = 0;

	protected bool m_wasOutsideAO = false;
	
	protected TILW_AOLimitDisplay m_display;
	
	protected IEntity m_controlledEntity;
	protected OnControlledEntityChangedPlayerControllerInvoker m_OnControlledEntityChanged;
	
	void TILW_AOLimitManager(IEntitySource src, IEntity parent)
	{
		s_Instance = this;
		
		if (RplSession.Mode() == RplMode.Dedicated)
			return;
		
		SetEventMask(EntityEvent.FIXEDFRAME);
	}
	
	protected void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		foreach(TILW_AOLimitComponent ao : m_aos)
			ao.OnEntityChanged();
	}
	
	protected override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		if (!m_OnControlledEntityChanged)
		{
			SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (!pc)
				return;
			
			m_OnControlledEntityChanged = pc.m_OnControlledEntityChanged;
			m_OnControlledEntityChanged.Insert(OnControlledEntityChanged);
		}
		
		if (m_wasOutsideAO)
			UpdateTimer(timeSlice);

		m_checkDelta -= timeSlice;
		if (m_checkDelta > 0)
			return;
		m_checkDelta = CHECKFREQUENCY;
		
		bool isPlayerInsideAOs = true;
		foreach(TILW_AOLimitComponent ao : m_aos)
		{
			if(!ao.IsPlayerSafe())
			{
				isPlayerInsideAOs = false;
				break;
			}
		}
		
		if(isPlayerInsideAOs)
			PlayerEntersAO();
		else
			PlayerLeavesAO();
	}
	
	void Register(TILW_AOLimitComponent ao)
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return;
		
		if(m_aos.Contains(ao))
			return;
		
		m_aos.Insert(ao);
		
		Print("TILW AO | Register " + ao);
	}
	
	void UnRegister(notnull TILW_AOLimitComponent ao)
	{
		if(!m_aos.Contains(ao))
			return;
		
		m_aos.RemoveItem(ao);
		
		Print("TILW AO | UnRegister " + ao);
	}
	
	static TILW_AOLimitManager GetInstance(bool create = true)
	{
		if (!s_Instance && create)
		{
			Print("TILW AO | Creating Manager");
			
			BaseWorld world = GetGame().GetWorld();
			if (world)
				s_Instance = TILW_AOLimitManager.Cast(GetGame().SpawnEntityPrefab(Resource.Load(PREFAB), world));
		}
		return s_Instance;
	}
	
	protected void UpdateTimer(float timeSlice)
	{
		m_timeLeft -= timeSlice;
		
		TILW_AOLimitDisplay display = GetDisplay();
		if(display)
			display.SetTime(m_timeLeft);

		if (m_timeLeft < 0)
		{
			IEntity player = GetGame().GetPlayerController().GetControlledEntity();
			CharacterControllerComponent characterController = CharacterControllerComponent.Cast(player.FindComponent(CharacterControllerComponent));
			if (characterController)
				characterController.ForceDeath();

			PlayerEntersAO();
		}
	}
	
	protected void PlayerLeavesAO()
	{
		if(m_wasOutsideAO)
			return;
		
		Print("TILW AO | Player leaves AO");
	
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		IEntity player = pc.GetControlledEntity();
		
		if(CompartmentAccessComponent.GetVehicleIn(player))
			m_timeLeft = m_vehicleKillTimer;
		else
			m_timeLeft = m_killTimer;
	
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.HINT);
			
		TILW_AOLimitDisplay display = GetDisplay();
		if(display)
		{
			display.SetTime(m_timeLeft);
			display.Show(true, UIConstants.FADE_RATE_INSTANT);
		}

		m_wasOutsideAO = true;
	}
	
	protected void PlayerEntersAO()
	{
		if(!m_wasOutsideAO)
			return;
		
		Print("TILW AO | Player enters AO");
		
		TILW_AOLimitDisplay display = GetDisplay();
		if (display)
			display.Show(false, UIConstants.FADE_RATE_INSTANT);
		
		m_timeLeft = m_killTimer;
		m_wasOutsideAO = false;
	}
	
	TILW_AOLimitDisplay GetDisplay()
	{
		if(m_display)
			return m_display;
		
		SCR_HUDManagerComponent hud = SCR_HUDManagerComponent.Cast(GetGame().GetPlayerController().GetHUDManagerComponent());
		m_display = TILW_AOLimitDisplay.Cast(hud.FindInfoDisplay(TILW_AOLimitDisplay));
		return m_display;
	}
}