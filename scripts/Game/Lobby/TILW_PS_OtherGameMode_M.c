#ifdef PS_LOBBY
	// Reforger Lobby is loaded (the framework depends on it) but the mission runs another game mode, such as
	// Coalition Lobby's. Parts of Reforger Lobby assume its own game mode is running and crash without it -
	// keep them out of the way. Nothing here changes a mission that uses Reforger Lobby.

	//------------------------------------------------------------------------------------------------
	class TILW_PS_OtherGameMode
	{
		//! Reforger Lobby is loaded but the running game mode isn't its PS_GameModeCoop
		static bool IsActive()
		{
			BaseGameMode gameMode = GetGame().GetGameMode();
			return gameMode && !PS_GameModeCoop.Cast(gameMode);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! A playable character registers with PS_GameModeCoop - under another game mode nothing is a Reforger Lobby slot
	modded class PS_PlayableComponent
	{
		override void LateInit()
		{
			// Spawned already marked playable (Reforger Lobby's *_P prefabs, e.g. as another lobby's slot characters)
			if (TILW_PS_OtherGameMode.IsActive())
				m_bIsPlayable = false;

			super.LateInit();
		}

		override void SetPlayable(bool isPlayable)
		{
			if (isPlayable && TILW_PS_OtherGameMode.IsActive())
				return;

			super.SetPlayable(isPlayable);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Reforger Lobby's slot groups ("Set all characters playable") left in a mission now run by another lobby: they
	//! are nobody's slots any more, so the group and its characters are removed rather than left standing as AI
	modded class SCR_AIGroup
	{
		override void MakePlayable(SCR_AIGroup group)
		{
			if (!TILW_PS_OtherGameMode.IsActive())
			{
				super.MakePlayable(group);
				return;
			}

			if (!Replication.IsServer())
				return;

			array<AIAgent> agents = {};
			GetAgents(agents);
			foreach (AIAgent agent : agents)
			{
				IEntity character = agent.GetControlledEntity();
				if (character)
					GetGame().GetCallqueue().Call(SCR_EntityHelper.DeleteEntityAndChildren, character);
			}

			GetGame().GetCallqueue().Call(SCR_EntityHelper.DeleteEntityAndChildren, this);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Reforger Lobby's freeze-time fire blocking on the player controller reads its game mode every frame
	modded class PS_PlayableControllerComponent
	{
		override protected void EOnFrame(IEntity owner, float timeSlice)
		{
			if (!PS_GameModeCoop.Cast(GetGame().GetGameMode()))
			{
				ClearEventMask(owner, EntityEvent.FRAME);
				return;
			}

			super.EOnFrame(owner, timeSlice);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Reforger Lobby's photo mode (ArmaVision) setting is read from its game mode whenever the photo mode is
	//! created - without that game mode there's no photo mode
	modded class SCR_EditorManagerEntity
	{
		override void SetEditorModes(EEditorModeAccess access, EEditorMode modes, bool isInit = false)
		{
			if (TILW_PS_OtherGameMode.IsActive())
				modes &= ~EEditorMode.PHOTO;

			super.SetEditorModes(access, modes, isInit);
		}
	}
#endif
