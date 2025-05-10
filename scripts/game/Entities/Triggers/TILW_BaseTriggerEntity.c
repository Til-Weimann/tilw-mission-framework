[EntityEditorProps(category: "GameScripted/Triggers", description: "The TILW_BaseTriggerEntity is a custom trigger designed to be used with the TILW_MissionFrameworkEntity. \nDo not use the base version directly..")]
class TILW_BaseTriggerEntityClass : GenericEntityClass
{
}

class TILW_BaseTriggerEntity : GenericEntity
{

	// QUERY SETTINGS

	[Attribute("25", UIWidgets.Auto, "Radius of sphere trigger. If the parent entity is a polyline with 3+ points, the radius ignored and the trigger operates in polyline mode.", category: "Trigger Query")]
	protected float m_queryRadius;

	[Attribute("10", UIWidgets.Auto, "Period of query in seconds. OBSOLETE, WILL BE REMOVED SOON.", category: "Deprecated", params: "0.25 inf 0.25")]
	protected float m_queryPeriod; // delete soon

	[Attribute("0", UIWidgets.Auto, "Skip the discovery query (which otherwise determines the initial state immediately, skipping capture iterations)", category: "Trigger Query")]
	protected bool m_skipFirstQuery;


	// CONDITION SETTINGS

	[Attribute("1", UIWidgets.ComboBox, "Select comparison mode", enums: ParamEnumArray.FromEnum(TILW_EComparisonMode), category: "Trigger Condition")]
	protected TILW_EComparisonMode m_comparisonMode;


	// STATUS SETTINGS

	[Attribute("1", UIWidgets.Auto, "OBSOLETE, WILL BE REMOVED SOON. Use capture time instead.", category: "Deprecated", params: "1 inf")]
	protected int m_captureIterations; // delete soon
	
	[Attribute("0", UIWidgets.Auto, "How many seconds it takes for the trigger state to change. \nWhen progress is interrupted, it starts ticking back down towards zero. \nIf 0, the trigger is captured instantly once the condition is met.", category: "Trigger Status", params: "0 inf")]
	protected float m_captureTime;

	[Attribute("0", UIWidgets.Auto, "Whether to display a message whenever the triggers capture status changes.", category: "Trigger Status")]
	protected bool m_sendStatusMessages;

	[Attribute("", UIWidgets.Auto, "Messages displayed when the triggers capture status changes. \nOnly modify, do not reorder or remove any of the 6 entries. \nEmpty messages will not be sent. \nFor the TILW_FactionTrigger, %1 will be replaced with the faction name. For both triggers, %2 will be replaced with the location name.", category: "Trigger Status")]
	protected ref array<string> m_statusMessages;

	[Attribute("the location", UIWidgets.Auto, "Name of the location/objective/etc. This will replace %2 in the status messages above.", category: "Trigger Status")]
	protected string m_locationName;


	// EFFECT SETTINGS

	[Attribute("", UIWidgets.Auto, "Set a flag when the condition becomes true (clear it when false)", category: "Trigger Effect")]
	protected string m_flagName;

	[Attribute("1", UIWidgets.Auto, "Only invoke SCRIPT events if the gamemode state is GAME (flags are still set, but not checked either way until game start - so mission events are not affected either way).", category: "Trigger Effect")]
	protected bool m_eventsOnlyDuringGame;

	[Attribute("0", UIWidgets.Auto, "After the effect is first triggered, prevent the trigger from doing any further queries. \nThis also prevents the flag from potentially being cleared again.", category: "Trigger Effect")]
	protected bool m_stopAfterFirstChange;


	// CLASS VARIABLES

	//! How many matching entities are currently in the trigger
	protected int m_totalCount;
	
	//! How many matching entities are currently in the trigger, which also fulfill a special condition (faction etc.)
	protected int m_specialCount;

	//! Current state of the trigger
	bool m_lastResult = false;

	//! Is this the first ever query?
	protected bool m_firstQuery = true;

	//! Current status of the trigger, used for status updates
	protected int m_currentStatus = -1;
	
	//! Is the trigger currently active?
	protected bool m_isActive = true;
	
	//! Is the trigger currently waiting for activation?
	protected bool m_isActivating = true;
	
	//! Copy of the polyline points
	protected ref array<vector> m_points3D = {};
	protected ref array<vector> m_pointsOverride = {};


	// ACCESS POINTS

	//! Event: Result changed - Occurs when the trigger condition was checked after a query, and the result is different from the last run \nTo be used by entity scripts
	event void OnResultChanged(bool result);

	protected ref ScriptInvokerBool m_OnResultChanged;
	
	//! Provides a ScriptInvoker for outsiders that want to subscribe to trigger result changes.
	ScriptInvokerBool GetOnResultChanged()
	{
		if (!m_OnResultChanged)
			m_OnResultChanged = new ScriptInvokerBool();
		return m_OnResultChanged;
	}
	
	//! Allows changing the query radius via script
	void SetRadius(float radius)
	{
		m_queryRadius = radius;
	}
	
	//! Allows overriding the triggers polyline points via script
	void SetPoints(array<vector> points3D)
	{
		m_pointsOverride = m_points3D;
	}
	
	//! Enable / disable the trigger. Activation does not take effect immediately.
	void SetActive(bool active)
	{
		if (!m_isActive && active)
			m_isActivating = true;
		else if (m_isActive && !active)
			m_isActive = false;
	}
	
	
	// INIT
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!Replication.IsServer())
			return;
		
		GetPolylinePoints();
		
		if (!GetGame().InPlayMode())
			return;
		
		if (m_skipFirstQuery)
			m_firstQuery = false;
		
		TILW_TriggerSystem.GetInstance().InsertTrigger(this);
	}
	
	void ~TILW_BaseTriggerEntity()
	{
		if (!GetGame().InPlayMode())
			return;
		
		TILW_TriggerSystem ts = TILW_TriggerSystem.GetInstance();
		if (ts)
			ts.RemoveTrigger(this);
	}


	// TRIGGER LOGIC
	
	//! Eval starts an evaluation round and resets counts
	void Eval()
	{
		// Activation and evaluation
		if (m_isActive)
			EvaluateState();
		else if (m_isActivating)
		{
			m_isActive = true;
			m_isActivating = false;
		}
		// Update polyline
		if (!m_pointsOverride.IsEmpty())
			m_points3D = m_pointsOverride;
		else
			GetPolylinePoints();
		// Reset count
		m_totalCount = 0;
		m_specialCount = 0;
	}
	
	protected int m_lastEvaluation = 0;
	protected int m_changeProgress = 0;

	//! EvaluateState is responsible for updating the state of the trigger
	protected void EvaluateState()
	{
		CustomQuery(); // Perform custom query if there is one
		
		int currentTime = System.GetTickCount();
		int deltaTime = currentTime - m_lastEvaluation;
		m_lastEvaluation = currentTime;
		
		float changeTime;
		if (m_captureTime == 0 && m_captureIterations > 1)
			changeTime = m_captureIterations * m_queryPeriod; // Still using the old parameters
		else	
			changeTime = m_captureTime;
		
		bool condition = EvaluateCondition();

		bool isDifferent = (condition != m_lastResult);
		bool shouldChange = isDifferent && ((m_changeProgress + deltaTime) >= changeTime * 1000) && !m_firstQuery;

		if (shouldChange) {
			// Result is changing now
			UpdateProgressStatus(0 + 3 * (int) ((m_lastResult && m_comparisonMode == 1) || (!m_lastResult && m_comparisonMode == 0)));
			m_changeProgress = 0;
		} else if (isDifferent && !m_firstQuery) {
			// Result is not yet changing, but eventually will if nothing changes
			UpdateProgressStatus(1 + 3 * (int) ((m_lastResult && m_comparisonMode == 1) || (!m_lastResult && m_comparisonMode == 0)));
			m_changeProgress += deltaTime; // Move towards change
		}
		if ((m_currentStatus == 1 || m_currentStatus == 4) && !isDifferent) {
			// Result is no longer projected to change
			UpdateProgressStatus(2 + 3 * (int) ((m_lastResult && m_comparisonMode == 1) || (!m_lastResult && m_comparisonMode == 0)));
			m_changeProgress = Math.Max(0, m_changeProgress - deltaTime); // Trend back towards 0
		}

		if (shouldChange || m_firstQuery) {
			TILW_MissionFrameworkEntity mfe = TILW_MissionFrameworkEntity.GetInstance();
			if (mfe)
				mfe.AdjustMissionFlag(m_flagName, condition, !m_firstQuery); // Update mission flag
			m_lastResult = condition;
		}

		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());

		if (shouldChange && (!m_eventsOnlyDuringGame || (gamemode && gamemode.GetState() == SCR_EGameModeState.GAME)))
		{
			GetGame().GetCallqueue().Call(OnResultChanged, condition); // Call Event method
			if (m_OnResultChanged)
				m_OnResultChanged.Invoke(condition); // Invoke ScriptInvoker
			if (m_stopAfterFirstChange)
				SetActive(false); // Perhaps stop doing queries
		}

		if (m_firstQuery)
			m_firstQuery = false;
	}
	
	// Could turn some of these into entity script events

	//! EvaluateCondition determines if the trigger condition is met. It should be overridden with some count comparison.
	protected bool EvaluateCondition();
	
	//! TotalFilter checks if the character is relevant to the trigger, e. g. being alive or inside of it
	protected bool TotalFilter(SCR_ChimeraCharacter cc);
	
	//! SpecialFilter checks if the character also fulfills a special condition, e. g. having some faction
	protected bool SpecialFilter(SCR_ChimeraCharacter cc);
	
	//! CustomQuery can be overridden with custom logic which is run before evaluation, independent of the trigger system
	protected void CustomQuery();
	
	//! ProcessCharacter passes the trigger characters, which are counted according to the filters
	void ProcessCharacter(SCR_ChimeraCharacter cc)
	{
		if (!m_isActive)
			return;
		if (!TotalFilter(cc))
			return;
		m_totalCount += 1;
		if (!SpecialFilter(cc))
			return;
		m_specialCount += 1;
	}

	
	// TRIGGER STATUS
	
	//! UpdateProgressStatus updated the capture status, and potentially sends a status message to players
	protected void UpdateProgressStatus(int status) // 0 = was set, 1 = is changing 2 = is no longer changing
	{
		if (status == m_currentStatus)
			return;

		if (m_sendStatusMessages) {
			string message = GetStatusMessage(status);
			if (message != "")
				SendStatusMessage(message);
		}
		m_currentStatus = status;
	}

	//! GetStatusMessage gets a status message from the array, with inserted location name.
	protected string GetStatusMessage(int status)
	{
		if (status > m_statusMessages.Count() - 1) {
			return "ERROR: Trigger Status Messages are not configured properly, there should be exactly 6 elements.";
		}
		return string.Format(m_statusMessages[status], "%1", m_locationName);
	}
	
	//! SendStatusMessage gets a status message from the array, with inserted location name.
	protected void SendStatusMessage(string message)
	{
		Print("TILWMF | Sending trigger status message: " + message);
		TILW_MissionFrameworkEntity mfe = TILW_MissionFrameworkEntity.GetInstance();
		array<string> fkeys = new array<string>;
		mfe.ShowGlobalHint("Objective Status", message, 5, fkeys);
	}
	
	
	// HELPERS
	
	//! Load points from the parent polyline, if one exists
	protected void GetPolylinePoints()
	{
		PolylineShapeEntity pse = PolylineShapeEntity.Cast(GetParent());
		if (!pse)
			return;
		m_points3D = {};
		pse.GetPointsPositions(m_points3D);
		for (int i = 0; i < m_points3D.Count(); i++)
			m_points3D[i] = pse.CoordToParent(m_points3D[i]);
	}
	
	//! Should the trigger operate in polyline mode?
	protected bool IsPolylineTrigger()
	{
		return (m_points3D.Count() >= 3);
	}
	
	//! Checks if a position is within the trigger shape
	protected bool IsWithinShape(vector pos)
	{
		if (!IsPolylineTrigger())
			return (vector.Distance(GetOrigin(), pos) <= m_queryRadius);
		return Math2D.IsPointInPolygonXZ(m_points3D, pos);
	}
	
	//! GetFactionKey gets a characters faction key. If it has no faction, "" is returned.
	static FactionKey GetFactionKey(SCR_ChimeraCharacter cc)
	{
		if (!cc.m_pFactionComponent)
			return "";
		Faction f = cc.m_pFactionComponent.GetAffiliatedFaction();
		if (!f)
			return "";
		return f.GetFactionKey();
	}
	
	//! IsEntityDestroyed checks if the entity is still alive. This works for anything with an SCR_DamageManagerComponent.
	static bool IsEntityDestroyed(IEntity entity)
	{
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(entity);
		return (damageManager && damageManager.GetState() == EDamageState.DESTROYED);
	}
	
	
	// WORKBENCH

	[Attribute("1", UIWidgets.Auto, "Draw faces of debug sphere", category: "Debug")]
	protected bool m_drawShapeSurface;

	[Attribute("1", UIWidgets.Auto, "Highlight edges of debug sphere", category: "Debug")]
	protected bool m_drawShapeOutline;

	[Attribute("0.25 0.5 0 1", UIWidgets.ColorPicker, "Color of debug sphere", category: "Debug")]
	protected ref Color m_setShapeColor;

#ifdef WORKBENCH

	override int _WB_GetAfterWorldUpdateSpecs(IEntitySource src)
	{
		return EEntityFrameUpdateSpecs.CALL_ALWAYS;
	}

	override void _WB_AfterWorldUpdate(float timeSlice)
	{
		super._WB_AfterWorldUpdate(timeSlice);
		GetPolylinePoints();
		if (!IsPolylineTrigger())
			DrawDebugShape();
		// GenerateAreaMesh
	}

	//! DrawDebugShape() draws a debug sphere, useful for previeving the query radius.
	protected void DrawDebugShape(bool polyline = false)
	{
		Shape dbgShape = null;

		Color c = m_setShapeColor;
		c.SetA(0.1);

		if (!m_drawShapeSurface && !m_drawShapeOutline)
			return;
		ShapeFlags flags = ShapeFlags.TRANSP | ShapeFlags.DOUBLESIDE | ShapeFlags.NOZWRITE | ShapeFlags.ONCE | ShapeFlags.NOCULL;
		if (!m_drawShapeSurface && m_drawShapeOutline)
			flags = flags | ShapeFlags.WIREFRAME;
		if (m_drawShapeSurface && !m_drawShapeOutline)
			flags = flags | ShapeFlags.NOOUTLINE;

		dbgShape = Shape.CreateSphere(c.PackToInt(), flags, GetOrigin(), m_queryRadius);
	}
	
#endif
	
}

enum TILW_EComparisonMode
{
	EQUAL_OR_LESS,
	EQUAL_OR_MORE
}

// Optionally, a bool that determines the receivers (global/local/faction) and notification color