modded enum ENotification
{
	TILW_SAFE_START
}

modded class SCR_CharacterControllerComponent
{
	static protected const int COOLDOWN = 1000;
	static protected const ref array<string> ACTIONS = { "TurretFire", "VehicleFire", "CharacterFire", "CharacterThrowGrenade", "CharacterMelee", "CharacterFireStatic" };
	protected int m_lastCalled = 0;
	protected bool m_safestartActive = true;
	
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		super.OnPrepareControls(owner, am, dt, player);
		
		#ifdef WORKBENCH
			m_safestartActive = false;
		#endif
		
		//if (m_safestartActive)
		//	HandleSafestart(am);
	}
	
	protected void HandleSafestart(ActionManager am)
	{
		if (!ShouldSafestartBeActive())
		{
			m_safestartActive = false;
			return;
		}
		
		foreach (string action : ACTIONS)
		{
			if (!am.GetActionTriggered(action))
				continue;
			
			am.SetActionValue(action, 0.0);
			NotifyPlayer();
			
			if (action != "CharacterFire")
				return;
		}
	}
	
	protected bool ShouldSafestartBeActive()
	{
		PS_GameModeCoop gm = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!gm || gm.m_safeStartTime <= 0)
			return false;
		
		float timeSeconds = gm.GetElapsedTime() - gm.GetGameStartElapsedTime();
		if (timeSeconds >= gm.m_safeStartTime)
			return false;

		return true;
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