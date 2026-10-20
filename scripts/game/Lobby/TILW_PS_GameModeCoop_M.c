#ifdef PS_LOBBY
	modded class PS_GameModeCoop : SCR_BaseGameMode
	{
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
	}
#endif
