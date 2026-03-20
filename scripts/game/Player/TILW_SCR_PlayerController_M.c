modded class SCR_PlayerController : PlayerController
{
	
	static SCR_PlayerController TILW_GetPCFromPID(int playerId)
	{
		return SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
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
		if(hintManager)
			return hintManager.Show(customHint, isSilent, ignoreShown);
		return false;
	}
	
	// ----- JIP TELEPORT -----------------------------------------------------------------------------------------------------------------------------------------------------------
	bool m_isJIPAvailable = false;
	protected static const float MAX_JIP_TIME = 120;
	
	override protected void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		
		GetGame().GetInputManager().AddActionListener("TILW_ManualJIP", EActionTrigger.DOWN, ManualJIP);
	}

	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);
		
		if(to.GetPrefabData().GetPrefabName() != "{ADDE38E4119816AB}Prefabs/InitialPlayer_Version2.et")
			CheckJIP();
	}
	
	protected void CheckJIP()
	{
		PS_GameModeCoop gm = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if(!gm || gm.GetState() != SCR_EGameModeState.GAME)
			return;

		float timeSeconds = gm.GetElapsedTime() - gm.GetGameStartElapsedTime();
		if(timeSeconds < gm.MIN_JIP_TIME)
			return;
	
		switch (gm.m_eJIPState)
		{
		    case EJIPState.Deny:
				return;
		
		    case EJIPState.Manual:
		        CanJIP();
				break;

		    case EJIPState.Auto:
		        TryJIP();
		        break;
		}
	}
	
	void CanJIP()
	{
		Rpc(RPC_CanJIP);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_CanJIP()
	{
		PS_GameModeCoop gm = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		EJIPResponse response = gm.CanJIP(this, null);
		HandleJIP(response);
	}

	protected void TryJIP()
	{
		Rpc(RPC_AskTryJIP);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_AskTryJIP()
	{
		PS_GameModeCoop gm = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if(!gm)
			return;
		
		EJIPResponse response = gm.TryJIP(this);
		
		HandleJIP(response);
	}
	
	void HandleJIP(EJIPResponse response)
	{
		Rpc(RPC_HandleJIP, response);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_HandleJIP(EJIPResponse response)
	{
		Print("TILWFW | JIP Response: " + response);
		
		switch(response)
		{
			case EJIPResponse.Disabled:
				JIPHint("JIP is disabled for this mission.");
				SetJIP(false);
				break;
			case EJIPResponse.Min:
				JIPHint("JIP is below the minimum time for this mission.");
				SetJIP(false);
				break;
			case EJIPResponse.Time:
				JIPHint("JIP is past the allowed time for this mission.");
				SetJIP(false);
				break;
			case EJIPResponse.InVehicle:
				JIPHint("Unable to JIP while in a Vehicle. Exit the vehicle and try to JIP.");
				SetJIP(true);
				break;
			case EJIPResponse.Group:
				JIPHint("No group available. Join a group and try to JIP.");
				SetJIP(true);
				break;
			case EJIPResponse.Target:
				JIPHint("No available JIP target. Please wait and try to JIP in a few seconds.");
				SetJIP(true);
				break;
			case EJIPResponse.Allowed:
				JIPHint("Double tap J to teleport JIP.", "JIP Available");
				SetJIP(true);
				break;
			case EJIPResponse.Sucess:
				SetJIP(false, true);
				break;
			default:
				SetJIP(false);
				break;
		}
	}
	
	void JIPHint(string msg, string title = "JIP Denied")
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		if(!menuManager)
			return;
		
		PS_SpectatorMenu spectatorMenu = PS_SpectatorMenu.Cast(menuManager.FindMenuByPreset(ChimeraMenuPreset.SpectatorMenu));
		if(spectatorMenu.IsOpen())
			return;
		
		PS_CoopLobby coopMenu = PS_CoopLobby.Cast(menuManager.FindMenuByPreset(ChimeraMenuPreset.CoopLobby));
		if(coopMenu.IsOpen())
			return;
		
		TILW_ShowHint(msg, title, 15);
	}
	
	protected void SetJIP(bool enabled, bool deactivate = false)
	{
		GetGame().GetCallqueue().Remove(SetJIP);
		
		if(enabled)
			GetGame().GetCallqueue().CallLater(SetJIP, MAX_JIP_TIME * 1000, false, false);
		
		if(deactivate)
		{
			PS_GameModeCoop gm = PS_GameModeCoop.Cast(GetGame().GetGameMode());
			gm.m_eJIPState = EJIPState.Deny;
		}
		
		m_isJIPAvailable = enabled;
	}
	
	protected void ManualJIP()
	{
		if(!m_isJIPAvailable)
			return;
		
		TryJIP();
	}
	
	void GetInVehicle(IEntity vehicle)
	{
		RplComponent rpl = RplComponent.Cast(vehicle.FindComponent(RplComponent));
		Rpc(RPC_GetInVehicle, rpl.Id());
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_GetInVehicle(RplId rplId)
	{
		IEntity entity = RplComponent.Cast(Replication.FindItem(rplId)).GetEntity();
		if(!entity)
			return;
		
		IEntity player = GetGame().GetPlayerController().GetControlledEntity();
		if(!player)
			return;
		
		SCR_CompartmentAccessComponent compartment = SCR_CompartmentAccessComponent.Cast(player.FindComponent(SCR_CompartmentAccessComponent));
		if(compartment.MoveInVehicleAny(entity))
			return;
	}
	
	void Teleport(vector transform[4])
	{
		Rpc(RPC_Teleport, transform);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_Teleport(vector transform[4])
	{
		IEntity player = GetControlledEntity();
		if(!player)
			return;
		
		BaseGameEntity baseGameEntity = BaseGameEntity.Cast(player);
		if(!baseGameEntity)
			return;
		
		baseGameEntity.Teleport(transform);
		
		Physics phys = player.GetPhysics();
		if(phys)
		{
			phys.SetVelocity(vector.Zero);
			phys.SetAngularVelocity(vector.Zero);
		}
	}
}

