enum TILW_EVisibilityMode
{
	ALL = 0,
	FACTION = 1,
	NONE = 2
}

class TILW_AOLimitManager : GameSystem
{
	protected const string PREFAB = "{42D6CF3CCE559B8C}Prefabs/Logic/AOLimit/TILW_AOLimitManager.et";
	protected const float CHECKFREQUENCY = 0.5;
	
	protected static TILW_AOLimitManager m_Instance;
	
	protected ref array<TILW_AOLimitComponent> m_aos = new array<TILW_AOLimitComponent>();
	
	protected float m_timeLeft = 0;
	protected float m_checkDelta = 0;

	protected bool m_wasOutsideAO = false;
	protected BaseWorld m_world;
	
	protected TILW_AOLimitDisplay m_display;
	protected OnControlledEntityChangedPlayerControllerInvoker m_OnControlledEntityChanged;
	
	override void OnInit()
	{
		super.OnInit();
		
		m_world = GetGame().GetWorld();
		Print("TILW | AO LIMIT OnInit");
		Enable(true);
	}
	
	override static void InitInfo(WorldSystemInfo outInfo)
	{
		super.InitInfo(outInfo);
		outInfo
			.SetAbstract(false)
			.SetUnique(true)
			.SetLocation(WorldSystemLocation.Client)
			.AddPoint(WorldSystemPoint.FixedFrame);
	}
	
	protected void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		m_OnControlledEntityChanged = null;
		foreach(TILW_AOLimitComponent ao : m_aos)
			ao.OnEntityChanged();
	}
	
	override protected void OnUpdate(ESystemPoint point)
	{
		super.OnUpdate(point);

		if (!m_OnControlledEntityChanged)
		{
			SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (!pc)
				return;
			
			m_OnControlledEntityChanged = pc.m_OnControlledEntityChanged;
			m_OnControlledEntityChanged.Insert(OnControlledEntityChanged);
		}
		
		float timeSlice = m_world.GetFixedTimeSlice();
		if (m_wasOutsideAO)
			UpdateTimer(timeSlice);

		m_checkDelta -= timeSlice;
		if (m_checkDelta > 0)
			return;
		m_checkDelta = CHECKFREQUENCY;
		
		bool isPlayerInsideAOs = true;
		TILW_AOLimitComponent leftAO;
		foreach(TILW_AOLimitComponent ao : m_aos)
		{
			if(!ao.IsPlayerSafe())
			{
				leftAO = ao;
				break;
			}
		}
		
		if(leftAO)
			PlayerLeavesAO(leftAO);
		else
			PlayerEntersAO();
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
		if(m_Instance)
			return m_Instance;
		
		World world = GetGame().GetWorld();
		if (!world)
			return null;
		
		return TILW_AOLimitManager.Cast(world.FindSystem(TILW_AOLimitManager));
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
	
	protected void PlayerLeavesAO(TILW_AOLimitComponent ao)
	{
		if(m_wasOutsideAO)
			return;
		
		Print("TILW AO | Player leaves AO: " + ao);
	
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		IEntity player = pc.GetControlledEntity();
		
		if(CompartmentAccessComponent.GetVehicleIn(player))
			m_timeLeft = ao.m_vehicleKillTimer;
		else
			m_timeLeft = ao.m_killTimer;
	
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
		
		m_timeLeft = 60;
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