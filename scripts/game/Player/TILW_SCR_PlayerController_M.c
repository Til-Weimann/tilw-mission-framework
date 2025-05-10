modded class SCR_PlayerController : PlayerController
{
	
	static SCR_PlayerController TILW_GetPCFromPID(int playerId)
	{
		return SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
	}
	
	// ----- FLAG REPLICATION -----------------------------------------------------------------------------------------------------------------------------------------------------------
	
	// JIP client requests all current flags from server
	void RequestMissionFlags()
	{
		Rpc(RpcAsk_RequestMissionFlags);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestMissionFlags()
	{
		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		if (fw)
			foreach (string name : fw.GetFlagSetCopy())
				Rpc(RpcDo_SetMissionFlag, name);
	};
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_SetMissionFlag(string name)
	{
		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		if (fw)
			fw.AdjustMissionFlag(name, true, true, true);
	}
	
	// ----- HINT MESSAGES -----------------------------------------------------------------------------------------------------------------------------------------------------------
	
	void TILW_SendHintToPlayer(string hl, string msg, int dur)
	{
		Rpc(TILW_RpcDo_ShowHint, hl, msg, dur);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void TILW_RpcDo_ShowHint(string hl, string msg, int dur)
	{
		TILW_ShowHint(msg, hl, dur);
	}
	
	protected bool TILW_ShowHint(string description, string name = string.Empty, float duration = 0, bool isSilent = false, EHint type = EHint.UNDEFINED, EFieldManualEntryId fieldManualEntry = EFieldManualEntryId.NONE, bool isTimerVisible = false, bool ignoreShown = true)
	{
		SCR_HintUIInfo customHint = SCR_HintUIInfo.CreateInfo(description, name, duration, type, fieldManualEntry, isTimerVisible);
		SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
		if (hintManager)
			return hintManager.Show(customHint, isSilent, ignoreShown);
		return false;
	}
	
	// ----- JIP TELEPORT -----------------------------------------------------------------------------------------------------------------------------------------------------------
	
	bool m_isJIPAvailable = false;
	protected int m_denyJIPManual = 120;
	
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);
		
		if(m_isJIPAvailable)
			RPC_DoSetJIP(false, string.Empty);
		
		GetGame().GetCallqueue().CallLater(CheckJIP, 1000, false);
	}
	
	protected void CheckJIP()
	{
		PS_GameModeCoop gm = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!gm)
			return;
		
		if(!gm.m_startTime)
			return;
		
		switch (gm.m_eJIPState)
		{
		    case EJIPState.Deny:
				return;
		
		    case EJIPState.Manual:
		        RPC_DoSetJIP(true, string.Empty);
				break;

		    case EJIPState.Auto:
		        if(CanJIP())
					TryJIP();
		        break;
		}
	}
	
	bool CanJIP()
	{
		PS_GameModeCoop gm = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!gm)
			return false;
		
		if(gm.m_eJIPState == EJIPState.Deny)
			return false;
		
		if(!gm.m_startTime)
			return false;
		
		ChimeraWorld world = GetGame().GetWorld();
		WorldTimestamp currentTime = world.GetServerTimestamp();
		if(currentTime.LessEqual(gm.m_startTime.PlusSeconds(120)))
			return false;

		if(gm.m_denyJIPTime)
		{
			if(currentTime.Greater(currentTime.PlusSeconds(gm.m_denyJIPTime)))
				return false;
		}
		
		CameraBase camera = GetGame().GetCameraManager().CurrentCamera();
		if(!PlayerCamera.Cast(camera))
			return false;
		
		IEntity player = GetGame().GetPlayerController().GetControlledEntity();
		if(!player)
			return false;
		
		IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(player);
		if(vehicle)
			return false;
		
		return true;
	}
	
	protected void ManualJIP()
	{
		if(CanJIP())
			TryJIP();
	}
	
	protected void TryJIP()
	{
		Rpc(RPC_AskTryJIP);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_AskTryJIP()
	{
		PS_GameModeCoop gm = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!gm)
			return;
		if(gm)
			gm.TryJIP(this);
	}
	
	void SetJIP(bool isAllowed, string msg = string.Empty)
	{
		Rpc(RPC_DoSetJIP, isAllowed, msg);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DoSetJIP(bool available, string msg)
	{
		// Print("TILW_JIPPlayerComponent.RPC_DoSetJIP");

		if(available)
		{
			if(m_isJIPAvailable)
			{
				GetGame().GetCallqueue().Remove(RPC_DoSetJIP);
				GetGame().GetCallqueue().CallLater(RPC_DoSetJIP, m_denyJIPManual * 1000, false, false, string.Empty);
			}
			else
			{
				GetGame().GetInputManager().AddActionListener("TILW_ManualJIP", EActionTrigger.DOWN, ManualJIP);
				GetGame().GetCallqueue().CallLater(RPC_DoSetJIP, m_denyJIPManual * 1000, false, false, string.Empty);
			}
		}
		else
		{
			GetGame().GetInputManager().RemoveActionListener("TILW_ManualJIP", EActionTrigger.DOWN, ManualJIP);
			GetGame().GetCallqueue().Remove(RPC_DoSetJIP);
		}
		
		m_isJIPAvailable = available;

		if(msg != string.Empty)
			TILW_ShowHint(msg, "JIP Denied");
	}
	
	void GetInVehicle(IEntity vehicle)
	{
		RplComponent rpl = RplComponent.Cast(vehicle.FindComponent(RplComponent));		
		
		Rpc(RPC_DoGetInVehicle, rpl.Id());
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DoGetInVehicle(RplId rplId)
	{
		IEntity entity = RplComponent.Cast(Replication.FindItem(rplId)).GetEntity();
		if(!entity)
			return;
		
		IEntity player = GetGame().GetPlayerController().GetControlledEntity();
		if(!player)
			return;
		
		SCR_CompartmentAccessComponent compartment = SCR_CompartmentAccessComponent.Cast(player.FindComponent(SCR_CompartmentAccessComponent));
		if(compartment.MoveInVehicleAny(entity))
			return RPC_DoSetJIP(false, string.Empty);
		
		RPC_DoSetJIP(true, "Unable to JIP into full vehicle.\n\nTry again in a bit!");
	}

	void Teleport(vector position)
	{
		Rpc(RPC_DoTeleport, position);
	}
	
    [RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DoTeleport(vector position)
	{
		IEntity player = GetGame().GetPlayerController().GetControlledEntity();
		if(!player)
			return;
		
		Physics physics = player.GetPhysics();
		if (physics)
		{
	        physics.SetVelocity(vector.Zero);
	        physics.SetAngularVelocity(vector.Zero);
		}
		
		player.SetOrigin(position);
		RPC_DoSetJIP(false, string.Empty);
	}
}

