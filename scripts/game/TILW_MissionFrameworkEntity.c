[EntityEditorProps(category: "GameScripted/", description:"TILW_MissionFrameworkEntity", dynamicBox: true)]
class TILW_MissionFrameworkEntityClass: GenericEntityClass
{
}
class TILW_MissionFrameworkEntity: GenericEntity
{
	
	// ----- INSTANCE MANAGEMENT -----------------------------------------------------------------------------------------------------------------------------------------------------------
	
	protected static TILW_MissionFrameworkEntity s_Instance = null;
	
	static TILW_MissionFrameworkEntity GetInstance(bool create = false)
	{
		if (!s_Instance && create) {
			Print("TILWMF | No MFE instance present, creating new one...");
			BaseWorld world = GetGame().GetWorld();
			if (world)
				s_Instance = TILW_MissionFrameworkEntity.Cast(GetGame().SpawnEntityPrefab(Resource.Load("{8F846D0FD5D6EA51}Prefabs/MP/TILW_MissionFrameworkEntity.et"), world));
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
		if (s_Instance == this)
			s_Instance = null;
		
		if (Replication.IsServer())
			RemoveListeners();
	}
	
	
	// ----- FLAG MANAGEMENT -----------------------------------------------------------------------------------------------------------------------------------------------------------
	
	protected ref set<string> m_flagSet = new set<string>();
	
	void AdjustMissionFlag(string name, bool value, bool recheck = true, bool asProxy = false)
	{
		if (m_rplComp.IsProxy() && !asProxy)
		{
			Print("TILWMF | Error: Proxy attempted to autonomously adjust a flag (" + name + ")!", LogLevel.ERROR);
			return;
		}
		
		if (IsMissionFlag(name) == value || name == "")
			return;
		
		if (value)
		{
			m_flagSet.Insert(name);
			Print("TILWMF | Set flag: " + name);
		}
		else
		{
			m_flagSet.RemoveItem(name);
			Print("TILWMF | Clear flag: " + name);
		}
		
		if (!m_rplComp.IsProxy())
		{
			Rpc(RpcDo_BroadcastFlagChange, name, value, recheck);
			if (recheck)
				RecheckConditions();
		}
		
		OnFlagChanged(name, value);
		if (m_OnFlagChanged)
			m_OnFlagChanged.Invoke(name, true);
	}
	
	bool IsMissionFlag(string name)
	{
		return m_flagSet.Contains(name);
	}
	
	void RecheckConditions()
	{	
		foreach (TILW_MetaFlag metaFlag : m_metaFlags)
			metaFlag.Evaluate();
		
		foreach (TILW_MissionEvent missionEvent : m_missionEvents)
			missionEvent.EvalExpression();
	}
	
	//! Returns a complete copy of the current flag set. For writing, always use AdjustMissionFlag! For reading individual flags, use IsMissionFlag.
	set<string> GetFlagSetCopy()
	{
		return set<string>.Cast(m_flagSet.Clone());
	}
	
	// ----- FLAG REPLICATION -----------------------------------------------------------------------------------------------------------------------------------------------------------
	
	
	protected RplComponent m_rplComp;
	
	protected void InitFlagReplication()
	{
		if (m_rplComp.IsProxy())
		{
			SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (!pc)
			{
				GetGame().GetCallqueue().CallLater(InitFlagReplication, 1000, false); // Wait for player controller
				return;
			}
			pc.RequestMissionFlags();
		}
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_BroadcastFlagChange(string name, bool value, bool recheck)
	{
		AdjustMissionFlag(name, value, recheck, true);
	};
	
	
	// ----- SCRIPTING SUPPORT -----------------------------------------------------------------------------------------------------------------------------------------------------------
	
	void SetPlayersKilledFlags(array<ref TILW_FactionPlayersKilledFlag> factionPlayersKilledFlags)
	{
		m_factionPlayersKilledFlags = factionPlayersKilledFlags;
	}
	
	void SetMissionEvents(array<ref TILW_MissionEvent> missionEvents)
	{
		m_missionEvents = missionEvents;
	}
	
	protected ref TILW_ScriptInvokerStringBool m_OnFlagChanged;
	
	//! Provides a ScriptInvoker for outsiders that want to subscribe to flag changes. \nInsertedMethod(string flagName, bool wasSetNotCleared);
	TILW_ScriptInvokerStringBool GetOnFlagChanged()
	{
		if (!m_OnFlagChanged)
			m_OnFlagChanged = new TILW_ScriptInvokerStringBool();
		return m_OnFlagChanged;
	}
	
	//! Event: OnFlagChanged, occurs whenever a mission flag changes. To be overridden by entity scripts.
	event void OnFlagChanged(string name, bool value);
	
	// some public variables usable in custom entity scripts. you can add more using a full entity script, these are just for convenience.
	int mvar_integer = 0;
	bool mvar_boolean = false;
	string mvar_string = "";
	
	
	// ----- ATTRIBUTES -----------------------------------------------------------------------------------------------------------------------------------------------------------
	
	// EVENTS
	
	[Attribute("", UIWidgets.Object, desc: "Mission Events can be triggered by combinations of flags, resulting in the execution of the events instructions", category: "Events")]
	ref array<ref TILW_MissionEvent> m_missionEvents;
	
	// FLAGS
	
	[Attribute("", UIWidgets.Object, desc: "Used to set a flags based on KIA ratios of factions", category: "Flags")]
	ref array<ref TILW_BaseCasualtyFlag> m_casualtyFlags;
	
	[Attribute("", UIWidgets.Object, desc: "Meta flags - these are defined as a combination of other flags. Useful for avoiding duplicates/redundancy of complex event conditions.", category: "Flags")]
	ref array<ref TILW_MetaFlag> m_metaFlags;
	
	[Attribute("", UIWidgets.Object, desc: "Random Flags are randomly set (or not set) before the mission starts.\nThey can e. g. be used to switch between two QRF events with different locations, based on whether a random flag was set or not.", category: "Flags")]
	ref array<ref TILW_BaseRandomFlag> m_randomFlags;
	
	[Attribute("", UIWidgets.Object, desc: "DEPRECATED - USE CASUALTY FLAGS INSTEAD\nUsed to set a flag when all players of a faction were killed", category: "Flags")] // TEMP
	ref array<ref TILW_FactionPlayersKilledFlag> m_factionPlayersKilledFlags;
	
	// DEBUG
	
	[Attribute("0", UIWidgets.Auto, desc: "Prevent the TILW_EndGameInstructions from actually ending the game, and display a hint message instead.", category: "Debug")]
	bool m_suppressGameEnd;
	

	// ----- SETUP -----------------------------------------------------------------------------------------------------------------------------------------------------------
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		Print("TILWMF | Framework EOnInit()");
		
		if (!GetGame().InPlayMode())
			return;
		
		m_rplComp = RplComponent.Cast(FindComponent(RplComponent));
		GetGame().GetCallqueue().CallLater(InitFlagReplication, 1000, false);
		
		if (m_rplComp.IsProxy())
			return;
		
		GetGame().GetCallqueue().Call(InsertListeners); // Insert listeners for player updates and game start
		GetGame().GetCallqueue().Call(InitRandomFlags); // Call, just to randomize time a little
		
		GetGame().GetCallqueue().CallLater(RecheckConditions, 10, false);
	}
	
	protected void InsertListeners()
	{
		PS_GameModeCoop gamemode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!gamemode)
			return;
		// gamemode.GetOnPlayerKilled().Insert(PlayerUpdate);
		gamemode.GetOnPlayerDeleted().Insert(PlayerUpdate);
		gamemode.GetOnPlayerSpawned().Insert(PlayerUpdate);
		gamemode.GetOnGameStateChange().Insert(GameStateChange);
	}
	
	protected void RemoveListeners()
	{
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gamemode)
			return;
		// gamemode.GetOnPlayerKilled().Remove(PlayerUpdate);
		gamemode.GetOnPlayerDeleted().Remove(PlayerUpdate);
		gamemode.GetOnPlayerSpawned().Remove(PlayerUpdate);
		gamemode.GetOnGameStart().Remove(GameStateChange);
	}
	
	protected void InitRandomFlags()
	{
		foreach (TILW_BaseRandomFlag rf : m_randomFlags)
			rf.Evaluate();
	}
	
	protected void GameStateChange(SCR_EGameModeState state)
	{
		if (state != SCR_EGameModeState.GAME)
			return;
		GetGame().GetCallqueue().CallLater(RecheckConditions, 10, false);
	}
	
	
	// ----- CASUALTY COUNTING -----------------------------------------------------------------------------------------------------------------------------------------------------------
	
	// PLAYERS
	
	protected bool m_playerRecountScheduled = false;
	ref map<string, int> m_curAliveFactionPlayers = new map<string, int>();
	ref map<string, int> m_maxAliveFactionPlayers = new map<string, int>();
	
	void PlayerUpdate(int playerId, IEntity player)
	{
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gamemode || gamemode.GetState() != SCR_EGameModeState.GAME)
			return;
		if (!m_playerRecountScheduled) {
			GetGame().GetCallqueue().CallLater(RecountPlayers, 5000, false); // Postpone it a bit, in case players don't have PCEs during loading. Perhaps not necessary.
			m_playerRecountScheduled = true;
		}
	}
	
	protected void RecountPlayers()
	{
		m_playerRecountScheduled = false;
		
		m_curAliveFactionPlayers.Clear();
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		foreach (int playerId : playerIds)
		{
			IEntity controlled = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!controlled)
				continue;
			SCR_ChimeraCharacter cc = SCR_ChimeraCharacter.Cast(controlled);
			if (!cc)
				continue;
			if (!SCR_AIDamageHandling.IsAlive(controlled))
				continue;
			Faction f = factionManager.GetPlayerFaction(playerId);
			if (!f)
				continue;
			string fkey = f.GetFactionKey();
			if (fkey == "")
				continue;
			m_curAliveFactionPlayers.Set(fkey, m_curAliveFactionPlayers.Get(fkey) + 1);
		}
		
		foreach (string f, int n : m_curAliveFactionPlayers) {
			if (n > m_maxAliveFactionPlayers.Get(f))
				m_maxAliveFactionPlayers.Set(f, n);
		}
		
		foreach (TILW_FactionPlayersKilledFlag fpkf : m_factionPlayersKilledFlags) // TEMP
			fpkf.Evaluate();
		
		foreach (TILW_BaseCasualtyFlag cf : m_casualtyFlags)
			if (TILW_FactionPlayersKilledFlag.Cast(cf))
				cf.Evaluate();
	}
	
	// AI
	
	protected bool m_aiRecountScheduled = false;
	ref map<string, int> m_factionAILifes = new map<string, int>();
	ref map<string, int> m_factionAIDeaths = new map<string, int>();
	
	void ScheduleRecountAI()
	{
		if (m_aiRecountScheduled)
			return;
		GetGame().GetCallqueue().CallLater(RecountAI, 1000, false);
		m_aiRecountScheduled = true;
	}
	
	protected void RecountAI()
	{
		m_aiRecountScheduled = false;
		
		foreach (TILW_BaseCasualtyFlag cf : m_casualtyFlags)
			if (TILW_FactionAIKilledFlag.Cast(cf))
				cf.Evaluate();
	}
	
	
	// ----- UTILS -----------------------------------------------------------------------------------------------------------------------------------------------------------
	
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
			if (!fm)
				return;
			Faction f = fm.GetLocalPlayerFaction();
			if (!f)
				return;
			if (!fkeys.Contains(f.GetFactionKey()))
				return;
		}
		ShowHint(msg, hl, dur);
	}
	
	protected bool ShowHint(string description, string name = string.Empty, float duration = 0, bool isSilent = false, EHint type = EHint.UNDEFINED, EFieldManualEntryId fieldManualEntry = EFieldManualEntryId.NONE, bool isTimerVisible = false, bool ignoreShown = true)
	{
		SCR_HintUIInfo customHint = SCR_HintUIInfo.CreateInfo(description, name, duration, type, fieldManualEntry, isTimerVisible);
		SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
		if (hintManager)
			return hintManager.Show(customHint, isSilent, ignoreShown);
		return false;
	}
	
}


// ----- META FLAGS -----------------------------------------------------------------------------------------------------------------------------------------------------------

[BaseContainerProps(), BaseContainerCustomTitleField("m_flagName")]
class TILW_MetaFlag
{
	[Attribute("", UIWidgets.Auto, desc: "New flag to be defined as a combination of other flags.")]
	protected string m_flagName;
	
	[Attribute("", UIWidgets.Object, desc: "Boolean expression defining the meta flag. \nConsists of (set when): Literal (flag), Conjunction (ALL operands), Disjunction (ANY operand), Minjunction (MIN operands), Maxjunction (MAX operands).")]
	ref TILW_BaseTerm m_definition;
	
	void Evaluate()
	{
		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		if (!fw)
			return;
		fw.AdjustMissionFlag(m_flagName, m_definition.Eval());
	}
}


// ----- CASUALTY FLAGS -----------------------------------------------------------------------------------------------------------------------------------------------------------

//! TILW_BaseCasualtyFlag is the base class for various player / AI casualty flags
[BaseContainerProps(), BaseContainerCustomStringTitleField("NOT USEFUL")]
class TILW_BaseCasualtyFlag
{
	[Attribute("", UIWidgets.Auto, desc: "Flag to be set when the faction reaches the given casualty ratio, or to be cleared when it's below. \nFactionPlayersKilledFlag: Only players are taken into account. \nFactionAIKilledFlag: Only AI is taken into account.")]
	protected string m_flagName;
	
	[Attribute("", UIWidgets.Auto, desc: "Key of examined faction")]
	protected string m_factionKey;
	
	[Attribute("1", UIWidgets.Auto, desc: "When the faction reaches/exceeds this casualty rate, the flag is set (e. g. 0.9 = 90% have been killed). \nFactionPlayersKilledFlag: Compares current alive players to historic simultaneous maximum of alive players. \nFactionAIKilledFlag: Compares AI deaths to (possibly non-simultaneous) AI lifes.", params: "0 1 0.01")]
	protected float m_casualtyRatio;
	
	void Evaluate();
	
	void SetFlag(string flagName)
	{
		m_flagName = flagName;
	}
		
	void SetKey(string key)
	{
		m_factionKey = key;
	}
		
	void SetCasualtyRatio(float ratio)
	{
		m_casualtyRatio = ratio;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Player Casualties Flag")]
class TILW_FactionPlayersKilledFlag : TILW_BaseCasualtyFlag
{
	override void Evaluate()
	{
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gamemode.GetState() != SCR_EGameModeState.GAME)
			return;
		
		TILW_MissionFrameworkEntity mfe = TILW_MissionFrameworkEntity.GetInstance();
		if (!GetGame().GetFactionManager().GetFactionByKey(m_factionKey))
			return;
		
		mfe.AdjustMissionFlag(m_flagName, mfe.m_curAliveFactionPlayers.Get(m_factionKey) <= mfe.m_maxAliveFactionPlayers.Get(m_factionKey) * (1 - m_casualtyRatio));
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("AI Casualties Flag")]
class TILW_FactionAIKilledFlag : TILW_BaseCasualtyFlag
{
	override void Evaluate()
	{
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gamemode.GetState() != SCR_EGameModeState.GAME)
			return;
		
		TILW_MissionFrameworkEntity mfe = TILW_MissionFrameworkEntity.GetInstance();
		if (!GetGame().GetFactionManager().GetFactionByKey(m_factionKey))
			return;
		
		float totalLifes = mfe.m_factionAILifes.Get(m_factionKey);
		float totalDeaths = mfe.m_factionAIDeaths.Get(m_factionKey);
		
		mfe.AdjustMissionFlag(m_flagName, totalLifes > 0 && totalDeaths / totalLifes >= m_casualtyRatio);
	}
}


// ----- RANDOM FLAGS -----------------------------------------------------------------------------------------------------------------------------------------------------------

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
		if (mfe && m_chance >= f)
			mfe.AdjustMissionFlag(m_flagName, true);
		// Fun fact: A chance of 0 can technically still set the flag, because RandomFloat01 is inclusive. I don't know an elegant way to fix this.
		// Anyway, this will never happen! Okay, maybe once in about 4 billion times...
		// I'm gonna print a cool message just in case!
		if (f == 0 && m_chance == 0)
			Print("You should have played lotto instead of Arma. Too bad!");
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
			if (m_flagNames.IsEmpty())
				break;
			string flag = m_flagNames.GetRandomElement();
			if (!m_withReplacement)
				m_flagNames.RemoveItem(flag);
			mfe.AdjustMissionFlag(flag, true);
		}
	}
}


// ----- WIP STUFF -----------------------------------------------------------------------------------------------------------------------------------------------------------

// Unused
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


// ----- OTHER STUFF -----------------------------------------------------------------------------------------------------------------------------------------------------------

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