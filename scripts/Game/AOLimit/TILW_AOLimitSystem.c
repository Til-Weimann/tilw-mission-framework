enum TILW_EVisibilityMode
{
	ALL = 0,
	FACTION = 1,
	NONE = 2
}

class TILW_AOLimitSystem : GameSystem
{
	protected const float CHECKFREQUENCY = 0.5;
	
	protected ref array<TILW_AOLimitComponent> m_aos = new array<TILW_AOLimitComponent>();
	
	protected float m_timeLeft = 0;
	protected float m_checkDelta = 0;

	protected bool m_wasOutsideAO = false;
	
	protected TILW_AOLimitDisplay m_display;
	protected OnControlledEntityChangedPlayerControllerInvoker m_OnControlledEntityChanged;
	
	override void OnInit()
	{
		super.OnInit();
		Enable(false); // will be enabled
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
		foreach (TILW_AOLimitComponent ao : m_aos)
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
		
		float timeSlice = GetGame().GetWorld().GetFixedTimeSlice();
		if (m_wasOutsideAO)
			UpdateTimer(timeSlice);

		m_checkDelta -= timeSlice;
		if (m_checkDelta > 0)
			return;
		m_checkDelta = CHECKFREQUENCY;
		
		bool isPlayerInsideAOs = true;
		TILW_AOLimitComponent leftAO;
		foreach (TILW_AOLimitComponent ao : m_aos)
		{
			if (!ao.IsPlayerSafe())
			{
				leftAO = ao;
				break;
			}
		}
		
		if (leftAO)
			PlayerOutsideAO(leftAO);
		else
			PlayerInsideAO();
	}
	
	void Register(TILW_AOLimitComponent ao)
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return;
		
		if (m_aos.Contains(ao))
			return;
		
		m_aos.Insert(ao);
		
		if (!IsEnabled())
			Enable(true);
	}
	
	void UnRegister(notnull TILW_AOLimitComponent ao)
	{
		if (!m_aos.Contains(ao))
			return;
		
		m_aos.RemoveItem(ao);
		
		if (m_aos.IsEmpty() && IsEnabled())
			Enable(false);
	}
	
	static TILW_AOLimitSystem GetInstance(bool create = true)
	{
		World world = GetGame().GetWorld();
		if (!world)
			return null;
		return TILW_AOLimitSystem.Cast(world.FindSystem(TILW_AOLimitSystem));
	}
	
	protected void UpdateTimer(float timeSlice)
	{
		m_timeLeft -= timeSlice;
		
		TILW_AOLimitDisplay display = GetDisplay();
		if (display)
			display.SetTime(m_timeLeft);

		if (m_timeLeft < 0)
		{
			IEntity player = GetGame().GetPlayerController().GetControlledEntity();
			CharacterControllerComponent characterController = CharacterControllerComponent.Cast(player.FindComponent(CharacterControllerComponent));
			if (characterController)
				characterController.ForceDeath();

			PlayerInsideAO();
		}
	}
	
	protected void PlayerOutsideAO(TILW_AOLimitComponent ao)
	{
		if (m_wasOutsideAO)
			return;
	
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		IEntity player = pc.GetControlledEntity();
		
		if (CompartmentAccessComponent.GetVehicleIn(player))
			m_timeLeft = ao.m_vehicleKillTimer;
		else
			m_timeLeft = ao.m_killTimer;
	
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.HINT);
			
		TILW_AOLimitDisplay display = GetDisplay();
		if (display)
		{
			display.SetTime(m_timeLeft);
			display.Show(true, UIConstants.FADE_RATE_INSTANT);
		}

		m_wasOutsideAO = true;
	}
	
	protected void PlayerInsideAO()
	{
		if (!m_wasOutsideAO)
			return;
		
		TILW_AOLimitDisplay display = GetDisplay();
		if (display)
			display.Show(false, UIConstants.FADE_RATE_INSTANT);
		
		m_timeLeft = 60; // why 60
		m_wasOutsideAO = false;
	}
	
	TILW_AOLimitDisplay GetDisplay()
	{
		if (m_display)
			return m_display;
		
		SCR_HUDManagerComponent hud = SCR_HUDManagerComponent.Cast(GetGame().GetPlayerController().GetHUDManagerComponent());
		m_display = TILW_AOLimitDisplay.Cast(hud.FindInfoDisplay(TILW_AOLimitDisplay));
		return m_display;
	}
}