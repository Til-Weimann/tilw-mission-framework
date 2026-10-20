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
}
