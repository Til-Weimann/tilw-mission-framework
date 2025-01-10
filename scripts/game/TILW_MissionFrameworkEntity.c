[EntityEditorProps(category: "GameScripted/", description:"TILW_MissionFrameworkEntity", dynamicBox: true)]
class TILW_MissionFrameworkEntityClass: GenericEntityClass
{
}
class TILW_MissionFrameworkEntity: GenericEntity
{
	
	// reference shit
	
	protected static TILW_MissionFrameworkEntity s_Instance = null;
	
	static TILW_MissionFrameworkEntity GetInstance(bool create = false)
	{
		if (!s_Instance && create) {
			Print("TILWMF | No MFE instance present, creating new one...");
			BaseWorld world = GetGame().GetWorld();
			if (world) s_Instance = TILW_MissionFrameworkEntity.Cast(GetGame().SpawnEntityPrefab(Resource.Load("{8F846D0FD5D6EA51}Prefabs/MP/TILW_MissionFrameworkEntity.et"), world));
		}
		return s_Instance;
	}
	
	static bool Exists()
	{
		return (s_Instance);
	}
	
	void TILW_MissionFrameworkEntity(IEntitySource src, IEntity parent)
	{
		s_Instance = this;
	}
	void ~TILW_MissionFrameworkEntity()
	{
		if (s_Instance == this) s_Instance = null;
	}
	
	
	// flag set
	
	protected ref set<string> m_flagSet = new set<string>();
	
	
	void SetMissionFlag(string name, bool recheck = true)
	{
		if (IsMissionFlag(name) || name == "") return;
		m_flagSet.Insert(name);
		Print("TILWMF | Set flag: " + name);
		if (recheck) {
			RecheckConditions();
			OnFlagChanged(name, true);
			if (m_OnFlagChanged) m_OnFlagChanged.Invoke(name, true);
		}
	}
	
	void ClearMissionFlag(string name, bool recheck = true)
	{
		if (!IsMissionFlag(name)) return;
		m_flagSet.RemoveItem(name);
		Print("TILWMF | Clear flag: " + name);
		if (recheck) {
			RecheckConditions();
			OnFlagChanged(name, false);
			if (m_OnFlagChanged) m_OnFlagChanged.Invoke(name, false);
		}
	}
	
	bool IsMissionFlag(string name)
	{
		return m_flagSet.Contains(name);
	}
	
	void AdjustMissionFlag(string name, bool setFlag, bool recheck = true)
	{
		if (setFlag) SetMissionFlag(name, recheck);
		else ClearMissionFlag(name, recheck);
	}
	
	void RecheckConditions()
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode.GetState() != SCR_EGameModeState.GAME) return;
		
		foreach (TILW_MissionEvent mEvent : m_missionEvents) mEvent.EvalExpression();
	}
	
	
	// events
	
	protected ref TILW_ScriptInvokerStringBool m_OnFlagChanged;
	
	//! Provides a ScriptInvoker for outsiders that want to subscribe to flag changes. \nInsertedMethod(string flagName, bool wasSetNotCleared);
	TILW_ScriptInvokerStringBool GetOnFlagChanged()
	{
		if (!m_OnFlagChanged) m_OnFlagChanged = new TILW_ScriptInvokerStringBool();
		return m_OnFlagChanged;
	}
	
	//! Event: OnFlagChanged, occurs whenever a mission flag changes. To be overridden by entity scripts.
	event void OnFlagChanged(string name, bool value);
	
	
	
	// some public variables usable in custom mission scripts. you can add more using an entity script.
	int mvar_counter = 0;
	bool mvar_boolean = false;
	
	
	// util variables
	protected bool m_recountScheduled = false;

	// pre-mission events
	[Attribute("", UIWidgets.Object, desc: "Events to trigger before advancing to game mode. Recommend not using for AI instructions.", category: "Events")]
	ref array<ref TILW_MissionEvent> m_preMissionEvents;	
	
	// mission events


	[Attribute("", UIWidgets.Object, desc: "Mission Events can be triggered by combinations of flags, resulting in the execution of the events instructions", category: "Events")]
	ref array<ref TILW_MissionEvent> m_missionEvents;
	
		
	// faction player count things
	
	[Attribute("", UIWidgets.Object, desc: "Used to set a flag when all players of a faction were killed", category: "Flags")]
	ref array<ref TILW_FactionPlayersKilledFlag> m_factionPlayersKilledFlags;
	
	// [Attribute("", UIWidgets.Auto, desc: "This array sets given flags on initialization. \nThis is not required for regular framework usage.", category: "Flags")]
	// ref array<string> m_defaultFlags;
	
	[Attribute("", UIWidgets.Object, desc: "Just like default flags, except that each one has a certain chance to be set on initialization.\nCan be used as a random factor, e. g. for switching between two QRF events with different locations, based on if a random flag was set or not.", category: "Flags")]
	ref array<ref TILW_BaseRandomFlag> m_randomFlags;
	
	
	// debug
	
	[Attribute("0", UIWidgets.Auto, desc: "Should TILW_EndGameInstructions be prevented from actually ending the game?", category: "Debug")]
	bool m_suppressGameEnd;
	
	// Initial setup
	
	override void EOnActivate(IEntity owner)
	{
		super.EOnActivate(owner);
		Print("TILWMF | Framework EOnActivate()");
		if (!Replication.IsServer()) return; // MFE only runs on server
		GetGame().GetCallqueue().Call(InsertListeners); // Insert listeners for player updates and game start
		GetGame().GetCallqueue().Call(InitDefaultFlags); // Call, just to randomize time a little
		foreach (TILW_MissionEvent mEvent : m_preMissionEvents) GetGame().GetCallqueue().Call(mEvent.EvalExpression); // Run pre-mission events
	}
	
	override void EOnDeactivate(IEntity owner)
	{
		super.EOnDeactivate(owner);
		Print("TILWMF | Framework EOnDeactivate()");
		if (!Replication.IsServer()) return;
		GetGame().GetCallqueue().Call(RemoveListeners);
	}
	
	protected void InsertListeners()
	{
		PS_GameModeCoop gamemode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!gamemode) return;
		// gamemode.GetOnPlayerKilled().Insert(PlayerUpdate);
		gamemode.GetOnPlayerDeleted().Insert(PlayerUpdate);
		gamemode.GetOnPlayerSpawned().Insert(PlayerUpdate);
		gamemode.GetOnGameStateChange().Insert(GameModeStart);
	}
	
	protected void RemoveListeners()
	{
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gamemode) return;
		// gamemode.GetOnPlayerKilled().Remove(PlayerUpdate);
		gamemode.GetOnPlayerDeleted().Remove(PlayerUpdate);
		gamemode.GetOnPlayerSpawned().Remove(PlayerUpdate);
		gamemode.GetOnGameStart().Remove(GameModeStart);
	}
	
	protected void InitDefaultFlags()
	{
		// foreach (string flag : m_defaultFlags) SetMissionFlag(flag, false);
		foreach (TILW_BaseRandomFlag rf : m_randomFlags) rf.Evaluate();
	}
	
	// Gamemode Start
	
	protected void GameModeStart(SCR_EGameModeState state)
	{
		if (state != SCR_EGameModeState.GAME) return;
		RecheckConditions();
	}
	
	void PlayerUpdate(int playerId, IEntity player)
	{
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gamemode || gamemode.GetState() != SCR_EGameModeState.GAME) return;
		if (!m_recountScheduled) {
			GetGame().GetCallqueue().CallLater(RecountPlayers, 5000, false); // Postpone it a bit, in case players don't have PCEs during loading. Perhaps not necessary.
			m_recountScheduled = true;
		}
	}
	
	ref map<string, int> m_curAliveFactionPlayers = new map<string, int>();
	ref map<string, int> m_maxAliveFactionPlayers = new map<string, int>();
	
	protected void RecountPlayers()
	{
		m_recountScheduled = false;
		
		m_curAliveFactionPlayers.Clear();
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		foreach (int playerId : playerIds)
		{
			IEntity controlled = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!controlled) continue;
			SCR_ChimeraCharacter cc = SCR_ChimeraCharacter.Cast(controlled);
			if (!cc) continue;
			if (!SCR_AIDamageHandling.IsAlive(controlled)) continue;
			
			Faction f = factionManager.GetPlayerFaction(playerId);
			if (!f) continue;
			string fkey = f.GetFactionKey();
			m_curAliveFactionPlayers.Set(fkey, m_curAliveFactionPlayers.Get(fkey) + 1);
		}
		
		foreach (string f, int n : m_curAliveFactionPlayers) {
			if (n > m_maxAliveFactionPlayers.Get(f)) m_maxAliveFactionPlayers.Set(f, n);
		}
		
		foreach (TILW_FactionPlayersKilledFlag fpkf : m_factionPlayersKilledFlags) fpkf.Evaluate();
	}
	
	
	
	// Utils
	
	void ShowGlobalHint(string hl, string msg, int dur, array<string> fkeys)
	{
		Rpc(RpcDo_ShowHint, hl, msg, dur, fkeys); // broadcast to clients
		RpcDo_ShowHint(hl, msg, dur, fkeys); // try to show on authority
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ShowHint(string hl, string msg, int dur, array<string> fkeys)
	{
		if (fkeys && !fkeys.IsEmpty()) {
			SCR_FactionManager fm = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			if (!fm) return;
			Faction f = fm.GetLocalPlayerFaction();
			if (!f) return;
			if (!fkeys.Contains(f.GetFactionKey())) return;
		}
		ShowHint(msg, hl, dur);
	}
	
	protected bool ShowHint(string description, string name = string.Empty, float duration = 0, bool isSilent = false, EHint type = EHint.UNDEFINED, EFieldManualEntryId fieldManualEntry = EFieldManualEntryId.NONE, bool isTimerVisible = false, bool ignoreShown = true)
	{
		SCR_HintUIInfo customHint = SCR_HintUIInfo.CreateInfo(description, name, duration, type, fieldManualEntry, isTimerVisible);
		SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
		if (hintManager) return hintManager.Show(customHint, isSilent, ignoreShown);
		return false;
	}
	
}


[BaseContainerProps(), BaseContainerCustomStringTitleField("FactionPlayersKilled Flag")]
class TILW_FactionPlayersKilledFlag
{
	[Attribute("", UIWidgets.Auto, desc: "Flag to be set when the faction reaches the given casualty ratio, or to be cleared when it's below. Only players are taken into account.")]
	protected string m_flagName;
	
	[Attribute("", UIWidgets.Auto, desc: "Key of examined faction")]
	protected string m_factionKey;
	
	[Attribute("1", UIWidgets.Auto, desc: "When the faction reaches/exceeds this casualty rate, the flag is set. \nTo be precise, when the current number of alive players divided by the historic maximum of concurrently alive players reaches this threshold. \nFor example, if the ratio is 0.9, the flag is set after 90% have been killed.", params: "0 1 0.01")]
	protected float m_casualtyRatio;
	
	void Evaluate()
	{
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gamemode.GetState() != SCR_EGameModeState.GAME) return;
		
		TILW_MissionFrameworkEntity mfe = TILW_MissionFrameworkEntity.GetInstance();
		if (!GetGame().GetFactionManager().GetFactionByKey(m_factionKey)) return;
		
		if (mfe.m_curAliveFactionPlayers.Get(m_factionKey) <= mfe.m_maxAliveFactionPlayers.Get(m_factionKey) * (1 - m_casualtyRatio)) mfe.SetMissionFlag(m_flagName);
		else mfe.ClearMissionFlag(m_flagName);
	}
}

//! TILW_BaseRandomFlag is the base class for various probability experiments that can set mission flags
[BaseContainerProps(), BaseContainerCustomStringTitleField("NOT USEFUL")]
class TILW_BaseRandomFlag
{
	void Evaluate();
}

//! TILW_RandomFlag is a simple Bernoulli trial, where a given mission flag has a given chance to be set
[BaseContainerProps(), BaseContainerCustomStringTitleField("Random Flag")]
class TILW_RandomFlag: TILW_BaseRandomFlag
{
	[Attribute("", UIWidgets.Auto, desc: "Mission Flag which might be set.")]
	protected string m_flagName;
	
	[Attribute("0.5", UIWidgets.Auto, desc: "Chance that the flag gets set on framework initialization", params: "0 1 0.05")]
	protected float m_chance;
	
	override void Evaluate()
	{
		TILW_MissionFrameworkEntity mfe = TILW_MissionFrameworkEntity.GetInstance();
		float f = Math.RandomFloat01();
		if (mfe && m_chance >= f) mfe.SetMissionFlag(m_flagName);
		// Fun fact: A chance of 0 can technically still set the flag, because RandomFloat01 is inclusive. I don't know an elegant way to fix this.
		// Anyway, this will never happen! Okay, maybe once in about 4 billion times...
		// I'm gonna print a cool message just in case!
		if (f == 0 && m_chance == 0) Print("You should have played lotto instead of Arma. Too bad!");
		// ...But why would you even add a flag with a chance of 0?
	}
}

//! TILW_RandomFlag is a simple Bernoulli trial, where a given mission flag has a given chance to be set
[BaseContainerProps(), BaseContainerCustomStringTitleField("Flag Sampling")]
class TILW_FlagSampling: TILW_BaseRandomFlag
{
	[Attribute("", UIWidgets.Auto, desc: "Pool to draw k random mission flags from, and set them.")]
	protected ref array<string> m_flagNames;
	
	[Attribute("1", UIWidgets.Auto, desc: "Number of flags to draw.", params: "1 inf")]
	protected int m_k;
	
	[Attribute("0", UIWidgets.Auto, desc: "Can a single entry be selected multiple times? \nThis means that the number of set flags can also be less than k, since a single one may be drawn multiple times.")]
	protected bool m_withReplacement;
	
	override void Evaluate()
	{
		TILW_MissionFrameworkEntity mfe = TILW_MissionFrameworkEntity.GetInstance();
		
		for (int k = m_k; k > 0; k--)
		{
			if (m_flagNames.IsEmpty()) break;
			string flag = m_flagNames.GetRandomElement();
			if (!m_withReplacement) m_flagNames.RemoveItem(flag);
			mfe.SetMissionFlag(flag);
		}
	}
}


class TILW_FactionRatioFlag
{
	[Attribute("", UIWidgets.Auto, desc: "Flag to be set/cleared when comparison returns true/false.")]
	protected string m_flagName;
	
	[Attribute(UIWidgets.Auto, desc: "Alive characters from these factions are counted, then compared to the total number of alive characters")]
	protected ref array<string> m_factionKeys;
	
	[Attribute("0", UIWidgets.Auto, desc: "Toggle comparison mode \nFalse means: Is factions share at or below threshold? \nTrue means: Is factions share at or above threshold?")]
	protected bool m_toggleMode;
	
	[Attribute("0", UIWidgets.Auto, desc: "Threshold used for comparison", params: "0 1 0.01")]
	protected float m_ratioThreshold;
	
	void Evalulate()
	{
		TILW_MissionFrameworkEntity mfe = TILW_MissionFrameworkEntity.GetInstance();
		float friendly = 0;
		float total = 0;
	}
}

enum TILW_EVariableOperation
{
	SET,
	ADD,
	SUB,
	MUL,
	DIV
}

// String-Bool Script Invoker for mission flag changes
void TILW_ScriptInvokerStringBoolMethod(string s, bool b);
typedef func TILW_ScriptInvokerStringBoolMethod;
typedef ScriptInvokerBase<TILW_ScriptInvokerStringBoolMethod> TILW_ScriptInvokerStringBool;

// List of things I may still work on:
// - Better notifications
// - Runtime displayed trigger (very optional)
// - Drawn AO Limit
// - Polyline triggers
// - Trigger which compares faction players in trigger to total amount of faction players
// - Entity Spawners