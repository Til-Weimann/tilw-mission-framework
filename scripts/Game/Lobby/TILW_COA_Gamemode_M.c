#ifdef COALITION_LOBBY
	//------------------------------------------------------------------------------------------------
	//! Coalition Lobby's game mode, for TILW_Lobby: its states reported as SCR_EGameModeState, and a player recount
	//! when someone dies
	modded class COA_Gamemode
	{
		protected ref ScriptInvokerInt m_TILW_OnGameStateChange;

		//------------------------------------------------------------------------------------------------
		ScriptInvokerInt TILW_GetOnGameStateChange()
		{
			if (!m_TILW_OnGameStateChange)
				m_TILW_OnGameStateChange = new ScriptInvokerInt();

			return m_TILW_OnGameStateChange;
		}

		//------------------------------------------------------------------------------------------------
		override protected void OnGamemodeStateChanged()
		{
			super.OnGamemodeStateChanged();

			SCR_EGameModeState state = SCR_EGameModeState.PREGAME;
			if (m_GamemodeState == COA_EGamemodeState.GAME)
				state = SCR_EGameModeState.GAME;
			else if (m_GamemodeState == COA_EGamemodeState.AAR)
				state = SCR_EGameModeState.POSTGAME;

			if (m_TILW_OnGameStateChange)
				m_TILW_OnGameStateChange.Invoke(state);
		}

		//------------------------------------------------------------------------------------------------
		//! The dead become spectators - recount the living (Reforger Lobby does this in TryRespawn)
		protected override void OnPlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
		{
			super.OnPlayerKilled(playerId, playerEntity, killerEntity, killer);

			TILW_MissionFrameworkEntity mfe = TILW_MissionFrameworkEntity.GetInstance();
			if (mfe)
				mfe.PlayerUpdate(0, null);
		}
	}
#endif
