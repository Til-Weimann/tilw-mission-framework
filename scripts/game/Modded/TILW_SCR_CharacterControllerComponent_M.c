modded enum ENotification
{
	TILW_SAFE_START
}

modded class SCR_CharacterControllerComponent
{
	static protected const int COOLDOWN = 1000;
	static protected const ref array<string> ACTIONS = { "TurretFire", "VehicleFire", "CharacterFire", "CharacterThrowGrenade", "CharacterMelee", "CharacterFireStatic" };
	protected int m_lastCalled;
	protected bool m_isActive = true;
	
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		super.OnPrepareControls(owner, am, dt, player);
		
		if(m_isActive)
			HandleAction(am);
	}
	
	protected void HandleAction(ActionManager am)
	{
		if(IsActionVaild())
		{
			m_isActive = false;
			return;
		}
		
		foreach (string action : ACTIONS)
		{
			if (!am.GetActionTriggered(action))
				continue;
			
			am.SetActionValue(action, 0.0);
			NotifyPlayer();
			
			if(action != "CharacterFire")
				return;
		}
	}
	
	protected bool IsActionVaild()
	{
		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		if(!fw || fw.m_safeStartTime == 0)
			return true;
		
		PS_GameModeCoop gm = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!gm)
			return true;
		
		if(!gm.m_startTime)
			return false;
		
		ChimeraWorld world = GetGame().GetWorld();
		WorldTimestamp currentTime = world.GetServerTimestamp();
		if(currentTime.Greater(gm.m_startTime.PlusSeconds(fw.m_safeStartTime)))
			return true;

		return false;
	}
	
	protected void NotifyPlayer()
	{
		int currentTick = System.GetTickCount();
		if (currentTick - m_lastCalled < COOLDOWN)
			return;
		
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.HINT);
		
		SCR_NotificationsComponent.SendToPlayer(SCR_PlayerController.GetLocalPlayerId(), ENotification.TILW_SAFE_START);
		m_lastCalled = currentTick;
	}
}