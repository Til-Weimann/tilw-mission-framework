//------------------------------------------------------------------------------------------------
//! Everything the framework needs from the lobby, for either lobby mod: Coalition Lobby (COALITION_LOBBY) or
//! Reforger Lobby (PS_LOBBY). Each method checks which lobby's game mode is running, so a server with both
//! mods loaded uses whichever the mission is built on. No other framework code touches a lobby directly.
class TILW_Lobby
{
	//------------------------------------------------------------------------------------------------
	//! The running lobby game mode, null when the mission doesn't use one
	static SCR_BaseGameMode GetGameMode()
	{
		#ifdef COALITION_LOBBY
			COA_Gamemode coaGamemode = COA_Gamemode.Cast(GetGame().GetGameMode());
			if (coaGamemode)
				return coaGamemode;
		#endif

		#ifdef PS_LOBBY
			PS_GameModeCoop psGamemode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			if (psGamemode)
				return psGamemode;
		#endif

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Game state changes (SCR_EGameModeState) - Coalition Lobby's briefing/slotting are PREGAME, its AAR POSTGAME
	static ScriptInvokerInt GetOnGameStateChange()
	{
		#ifdef COALITION_LOBBY
			COA_Gamemode coaGamemode = COA_Gamemode.Cast(GetGame().GetGameMode());
			if (coaGamemode)
				return coaGamemode.TILW_GetOnGameStateChange();
		#endif

		#ifdef PS_LOBBY
			PS_GameModeCoop psGamemode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			if (psGamemode)
				return psGamemode.GetOnGameStateChange();
		#endif

		return null;
	}

	// ----- Players and characters ---------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! The faction the lobby has the player in, empty if none
	static FactionKey GetPlayerFactionKey(int playerId)
	{
		#ifdef COALITION_LOBBY
			if (COA_Gamemode.Cast(GetGame().GetGameMode()))
			{
				COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
				if (!slottingManager)
					return "";

				Faction faction = slottingManager.GetPlayerSlotFaction(playerId, true);
				if (!faction)
					return "";

				return faction.GetFactionKey();
			}
		#endif

		#ifdef PS_LOBBY
			PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
			if (playableManager)
				return playableManager.GetPlayerFactionKey(playerId);
		#endif

		return "";
	}

	//------------------------------------------------------------------------------------------------
	//! Calls the AO limit back when the local player's faction may have changed: FactionChange(playerId, new, old)
	//! from Reforger Lobby, LobbySlottingChanged() from Coalition Lobby (which reports slotting updates instead)
	static void AddFactionChangeListener(notnull TILW_AOLimitComponent listener)
	{
		#ifdef COALITION_LOBBY
			if (COA_Gamemode.Cast(GetGame().GetGameMode()))
			{
				COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
				if (slottingManager)
					slottingManager.GetOnSlottingUpdate().Insert(listener.LobbySlottingChanged);
				return;
			}
		#endif

		#ifdef PS_LOBBY
			PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
			if (playableManager)
				playableManager.GetOnFactionChange().Insert(listener.FactionChange);
		#endif
	}

	//------------------------------------------------------------------------------------------------
	//! A lobby spectator (Coalition Lobby's spectators are characters in the world with their slot's faction)
	static bool IsSpectator(IEntity entity)
	{
		#ifdef COALITION_LOBBY
			if (entity && COA_Gamemode.Cast(GetGame().GetGameMode()))
				return COA_EntityHelper.IsSpectator(entity);
		#endif

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! A lobby slot's character nobody is playing (Reforger Lobby's unclaimed playables, Coalition Lobby's
	//! characters of players who left, and spectators) - not mission AI
	static bool IsUnplayedSlotCharacter(IEntity entity)
	{
		if (!entity)
			return false;

		#ifdef COALITION_LOBBY
			if (COA_Gamemode.Cast(GetGame().GetGameMode()))
			{
				if (COA_EntityHelper.IsSpectator(entity))
					return true;

				COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
				return slottingManager && slottingManager.GetCharacterSlotID(entity) != -1;
			}
		#endif

		#ifdef PS_LOBBY
			PS_PlayableComponent playable = PS_PlayableComponent.Cast(entity.FindComponent(PS_PlayableComponent));
			return playable && playable.GetPlayable();
		#endif

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Does the character count towards a faction's AI lives (mission AI, not a lobby slot)
	static bool IsMissionAI(IEntity entity)
	{
		if (!entity)
			return false;

		#ifdef COALITION_LOBBY
			if (COA_Gamemode.Cast(GetGame().GetGameMode()))
				return !IsUnplayedSlotCharacter(entity);
		#endif

		#ifdef PS_LOBBY
			PS_PlayableComponent playable = PS_PlayableComponent.Cast(entity.FindComponent(PS_PlayableComponent));
			return playable && !playable.GetPlayable();
		#endif

		return true;
	}

	// ----- Slots and respawns -------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Takes the slots nobody has claimed out of the game (all factions if factionKeys is empty)
	static void RemoveOpenSlots(notnull array<string> factionKeys)
	{
		#ifdef COALITION_LOBBY
			if (COA_Gamemode.Cast(GetGame().GetGameMode()))
			{
				// Coalition Lobby: lock them, so nobody can join into them
				COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
				if (!slottingManager)
					return;

				foreach (int slotId : slottingManager.GetAllSlotIds())
				{
					COA_SlotData slotData = slottingManager.GetSlotData(slotId);
					if (!slotData || slotData.GetSlotCurrentPlayerId() > 0)
						continue;

					if (!factionKeys.IsEmpty() && !factionKeys.Contains(slotData.GetSlotFactionKey()))
						continue;

					slottingManager.UpdateSlotLockedState(slotId, true);
				}
				return;
			}
		#endif

		#ifdef PS_LOBBY
			// Reforger Lobby: delete the unclaimed playable characters
			PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
			if (!playableManager)
				return;

			foreach (RplId id, PS_PlayableContainer container : playableManager.GetPlayables())
			{
				if (!container)
					continue;

				PS_PlayableComponent playable = container.GetPlayableComponent();
				if (!playable || playableManager.GetPlayerByPlayable(playable.GetRplId()) != -1)
					continue;

				IEntity entity = playable.GetOwner();
				if (!factionKeys.IsEmpty())
				{
					SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
					if (!character || !character.m_pFactionComponent)
						continue;

					Faction faction = character.m_pFactionComponent.GetAffiliatedFaction();
					if (!faction || !factionKeys.Contains(faction.GetFactionKey()))
						continue;
				}

				SCR_EntityHelper.DeleteEntityAndChildren(entity);
			}
		#endif
	}

	//------------------------------------------------------------------------------------------------
	static int GetFactionTickets(FactionKey factionKey)
	{
		#ifdef COALITION_LOBBY
			if (COA_Gamemode.Cast(GetGame().GetGameMode()))
			{
				COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
				if (respawnManager)
					return respawnManager.GetFactionTickets(factionKey);

				return 0;
			}
		#endif

		#ifdef PS_LOBBY
			PS_GameModeCoop psGamemode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			if (psGamemode)
				return psGamemode.TILW_GetFactionTicketCount(factionKey);
		#endif

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	static void SetFactionTickets(FactionKey factionKey, int tickets)
	{
		#ifdef COALITION_LOBBY
			if (COA_Gamemode.Cast(GetGame().GetGameMode()))
			{
				COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
				if (!respawnManager)
					return;

				int change = tickets - respawnManager.GetFactionTickets(factionKey);
				if (change > 0)
					respawnManager.AddTicket(factionKey, change, true);
				else if (change < 0)
					respawnManager.SubtractTicket(factionKey, -change, true);
				return;
			}
		#endif

		#ifdef PS_LOBBY
			PS_GameModeCoop psGamemode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			if (psGamemode)
				psGamemode.TILW_SetFactionTicketCount(factionKey, tickets);
		#endif
	}

	//------------------------------------------------------------------------------------------------
	//! Respawns players who died and are spectating (Reforger Lobby: all factions; Coalition Lobby: this faction,
	//! paid for from its tickets as usual)
	static void RespawnDeadPlayers(FactionKey factionKey)
	{
		#ifdef COALITION_LOBBY
			if (COA_Gamemode.Cast(GetGame().GetGameMode()))
			{
				COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
				if (respawnManager)
					respawnManager.RespawnSide(factionKey);
				return;
			}
		#endif

		#ifdef PS_LOBBY
			PS_GameModeCoop psGamemode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
			if (!psGamemode || !playableManager)
				return;

			foreach (PS_PlayableContainer container : playableManager.GetPlayablesSorted())
			{
				PS_PlayableComponent playable = container.GetPlayableComponent();
				SCR_CharacterDamageManagerComponent damageManager = playable.GetCharacterDamageManagerComponent();
				if (damageManager.GetState() != EDamageState.DESTROYED)
					continue;

				int playerId = playableManager.GetPlayerByPlayableRemembered(playable.GetRplId());
				if (playerId == -1)
					continue;

				psGamemode.TryRespawn(playable.GetRplId(), playerId);
			}
		#endif
	}

	// ----- Map items ----------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Shows or hides a lobby map marker per faction - Coalition Lobby's COA_ManualMarker, Reforger Lobby's
	//! PS_ManualMarker or PS_MissionDescription (Coalition Lobby's mission descriptions are configs, not entities)
	static void SetMapItemVisibility(IEntity item, bool visibleForEmpty, array<ref TILW_FactionVisibility> factionVisibility)
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());

		#ifdef COALITION_LOBBY
			COA_ManualMarker coaMarker = COA_ManualMarker.Cast(item);
			if (coaMarker)
			{
				coaMarker.SetVisibleForEmptyFaction(visibleForEmpty);
				if (!factionVisibility || !factionManager)
					return;

				foreach (TILW_FactionVisibility visibility : factionVisibility)
				{
					coaMarker.SetVisibleForFaction(factionManager.GetFactionByKey(visibility.m_factionKey), visibility.m_visible);
				}
				return;
			}
		#endif

		#ifdef PS_LOBBY
			PS_ManualMarker marker = PS_ManualMarker.Cast(item);
			PS_MissionDescription description = PS_MissionDescription.Cast(item);
			if (marker)
				marker.SetVisibleForEmptyFaction(visibleForEmpty);
			if (description)
				description.SetVisibleForEmptyFaction(visibleForEmpty);

			if (!factionVisibility || !factionManager)
				return;

			foreach (TILW_FactionVisibility visibility : factionVisibility)
			{
				Faction faction = factionManager.GetFactionByKey(visibility.m_factionKey);
				if (marker)
					marker.SetVisibleForFaction(faction, visibility.m_visible);
				if (description)
					description.SetVisibleForFaction(faction, visibility.m_visible);
			}
		#endif
	}
}
