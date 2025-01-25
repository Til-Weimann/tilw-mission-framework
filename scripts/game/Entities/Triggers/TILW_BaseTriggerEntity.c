[EntityEditorProps(category: "GameScripted/Triggers", description: "The TILW_BaseTriggerEntity is a custom trigger designed to be used with the TILW_MissionFrameworkEntity. \nDo not use the base version directly..")]
class TILW_BaseTriggerEntityClass: GenericEntityClass
{
}

class TILW_BaseTriggerEntity : GenericEntity
{
	
	// QUERY SETTINGS
	
	[Attribute("25", UIWidgets.Auto, "Radius of sphere query", category: "Trigger Query")]
	protected float m_queryRadius;
	
	[Attribute("10", UIWidgets.Auto, "Period of query in seconds", category: "Trigger Query", params: "0.25 inf 0.25")]
	protected float m_queryPeriod;
	
	[Attribute("1", UIWidgets.Auto, "Only run on the server.", category: "Trigger Query")]
	protected bool m_onlyOnServer;
	
	
	// FILTER SETTINGS
	
	[Attribute("", UIWidgets.ResourceAssignArray, "If defined, only these specific character prefabs (and potential children) will be taken into account.", "et", category: "Trigger Filter")]
	protected ref array<ResourceName> m_prefabFilter;
	
	
	// CONDITION SETTINGS
	
	[Attribute("1", UIWidgets.ComboBox, "Select comparison mode", enums: ParamEnumArray.FromEnum(TILW_EComparisonMode), category: "Trigger Condition")]
	protected TILW_EComparisonMode m_comparisonMode;
	
	
	// STATUS SETTINGS
	
	[Attribute("1", UIWidgets.Auto, "For how many query periods the condition has to be different from the current result, in order for the result to change. \nCan be used for capture timers (time = Capture Iterations * Query Period). \nWhile not different, any change progress slowly returns to 0. \nIf 1, the trigger is captured instantly once the condition is met.", category: "Trigger Status", params: "1 inf")]
	protected int m_captureIterations;
	
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

	[Attribute("0", UIWidgets.Auto, "Skip the first query, instead of evaluating the condition and setting the flag on first run", category: "Trigger Effect")]
	protected bool m_skipFirstQuery;
	
	
	// CLASS VARIABLES
	
	//! How many matching entities are currently in the trigger
	protected int m_totalCount;
	
	//! How many matching entities are currently in the trigger, which also fulfill a special condition (faction etc.)
	protected int m_specialCount;
	
	//! Current state of the trigger
	bool m_lastResult = false;
	
	//! Is this the first ever query?
	protected bool m_firstQuery = !m_skipFirstQuery;
	
	//! For how many iterations the condition has been different from the current result
	protected int m_currentIterations = 0;
	
	//! Current status of the trigger, used for status updates
	protected int m_currentStatus = -1;
	
	
	
	// EVENTS
	
	//! Event: Result changed - Occurs when the trigger condition was checked after a query, and the result is different from the last run \nTo be used by entity scripts
	event void OnResultChanged(bool result);
	
	protected ref ScriptInvokerBool m_OnResultChanged;
	
	//! Provides a ScriptInvoker for outsiders that want to subscribe to trigger result changes.
	ScriptInvokerBool GetOnResultChanged()
	{
		if (!m_OnResultChanged) m_OnResultChanged = new ScriptInvokerBool();
		return m_OnResultChanged;
	}
	
	
	// TRIGGER LOGIC
		
	override void EOnActivate(IEntity owner)
	{
		super.EOnActivate(owner);
		// Call 5s later because AI may not have been spawned yet
		// Better solution might be to only init the loop on game start - NOT. Since events could then be checked before all default queries are done.
		if (!m_onlyOnServer || Replication.IsServer()) GetGame().GetCallqueue().CallLater(QueryLoop, 5 * 1000, false);
	}
	
	// QueryLoop() is responsible for everything that happens within each query cycle
	protected void QueryLoop()
	{
		RunQuery();
		bool condition = EvaluateCondition();
		
		bool difference = (condition != m_lastResult);
		bool shouldChange = difference && (m_currentIterations + 1 >= m_captureIterations) && !m_firstQuery; // changed
		
		// changed - when shouldChange is true, old is not new, and it's not the first query
		// changing - when old is not new, but shouldChange is false
		// no longer changing - happens when old status was changing, but now old is new again
		
		if (shouldChange) {
			// Result is changing now
			UpdateProgressStatus(0 + 3 * (int) ( (m_lastResult && m_comparisonMode == 1) || (!m_lastResult && m_comparisonMode == 0) ) );
			m_currentIterations = 0;
		} else if (difference && !m_firstQuery) {
			// Result is not yet changing, but soon will if nothing changes
			UpdateProgressStatus(1 + 3 * (int) ( (m_lastResult && m_comparisonMode == 1) || (!m_lastResult && m_comparisonMode == 0) ) );
			m_currentIterations += 1;
		}
		if ((m_currentStatus == 1 || m_currentStatus == 4) && !difference) {
			// Result is now not projected to change, but was before
			UpdateProgressStatus(2 + 3 * (int) ( (m_lastResult && m_comparisonMode == 1) || (!m_lastResult && m_comparisonMode == 0) ) );
			m_currentIterations = Math.Max(0,m_currentIterations-1);
		}
		
		if (shouldChange || m_firstQuery) {
			TILW_MissionFrameworkEntity mfe = TILW_MissionFrameworkEntity.GetInstance();
			if (mfe) mfe.AdjustMissionFlag(m_flagName, condition, !m_firstQuery); // Update mission flag
			m_lastResult = condition;
		}
		
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		
		if (shouldChange && (!m_eventsOnlyDuringGame || (gamemode && gamemode.GetState() == SCR_EGameModeState.GAME)))
		{
			GetGame().GetCallqueue().Call(OnResultChanged, condition); // Call Event method
			if (m_OnResultChanged) m_OnResultChanged.Invoke(condition); // Invoke ScriptInvoker
			if (m_stopAfterFirstChange) return; // Perhaps stop doing queries
		}
		
		if (m_firstQuery) m_firstQuery = false;
		GetGame().GetCallqueue().CallLater(QueryLoop, m_queryPeriod * 1000, false);
	}
	
	//! RunQuery() resets the count and starts a new query
	protected void RunQuery()
	{
		m_totalCount = 0;
		m_specialCount = 0;
		GetGame().GetWorld().QueryEntitiesBySphere(GetOrigin(), m_queryRadius, AddEntity, FilterEntity);
	}
	
	
	//! EvaluateCondition checks if the trigger condition is true, and returns the result.
	protected bool EvaluateCondition();
	
	
	// HELPER METHODS
	
	//! AddEntity determines what happens when an entity has passed through the filter.
	protected bool AddEntity(IEntity e);
	
	//! FilterEntity is responsible for filtering out entities that do not meet the triggers requirements.
	protected bool FilterEntity(IEntity e)
	{
		if (!IsMatchingClass(e)) return false;
		if (!IsMatchingPrefab(e)) return false;
		
		return true;
	}
	
	//! IsMatchingClass can be overwritten to check if the entity is of a certain class, intended for filtering.
	protected bool IsMatchingClass(IEntity e);
	
	//! IsMatchingPrefab checks if the entity was created from a prefab included in the m_prefabFilter array.
	protected bool IsMatchingPrefab(IEntity e)
	{
		if (m_prefabFilter.IsEmpty()) return true;
		
		EntityPrefabData epd = e.GetPrefabData();
		if (!epd) return false;
		BaseContainer bc = epd.GetPrefab();
		if (!bc) return false;
		
		foreach (ResourceName rn : m_prefabFilter)
		{
			if (SCR_BaseContainerTools.IsKindOf(bc, rn)) return true;
		}
			
		return false;
	}
	
	//! IsEntityDestroyed checks if the entity is still alive. This works for anything with an SCR_DamageManagerComponent.
	protected bool IsEntityDestroyed(IEntity entity)
	{
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(entity);
		return (damageManager && damageManager.GetState() == EDamageState.DESTROYED);
	}
	
	//! UpdateProgressStatus updated the capture status, and potentially sends a status message to players
	protected void UpdateProgressStatus(int status) // 0 = was set, 1 = is changing 2 = is no longer changing
	{	
		if (status == m_currentStatus) return;
		
		if (m_sendStatusMessages) {
			string message = GetStatusMessage(status);
			if (message != "") SendStatusMessage(message);
		}
		m_currentStatus = status;
	}
	
	protected string GetStatusMessage(int status)
	{
		if (status > m_statusMessages.Count() - 1) {
			return "ERROR: Trigger Status Messages are not configured properly, there should be exactly 6 elements.";
		}
		return string.Format(m_statusMessages[status], "%1", m_locationName);
	}
	
	protected void SendStatusMessage(string message)
	{
		Print("TILWMF | Sending trigger status message: " + message);
		TILW_MissionFrameworkEntity mfe = TILW_MissionFrameworkEntity.GetInstance();
		array<string> fkeys = new array<string>;
		mfe.ShowGlobalHint("Objective Status", message, 5, fkeys);
	}
	
	// DEBUG
	
	[Attribute("1", UIWidgets.Auto, "Draw faces of debug sphere", category: "Debug")]
	protected bool m_drawShapeSurface;
	
	[Attribute("1", UIWidgets.Auto, "Highlight edges of debug sphere", category: "Debug")]
	protected bool m_drawShapeOutline;
	
	[Attribute("0.25 0.5 0 1", UIWidgets.ColorPicker, "Color of debug sphere", category: "Debug")]
	protected ref Color m_setShapeColor;
	
	//[Attribute("0", UIWidgets.Auto, "Also display sphere ingame (vs. just in the World Editor)", category: "Debug")]
	protected bool m_drawShapeIngame;
	
	override void _WB_AfterWorldUpdate(float timeSlice)
	{
		super._WB_AfterWorldUpdate(timeSlice);
		DrawDebugShape();
	}
	
	//! DrawDebugShape() draws a debug sphere, useful for previeving the query radius.
	protected void DrawDebugShape()
	{
		Shape dbgShape = null;
		
		Color c = m_setShapeColor;
		c.SetA(0.1);
		
		if (!m_drawShapeSurface && !m_drawShapeOutline) return;
		ShapeFlags flags = ShapeFlags.TRANSP | ShapeFlags.DOUBLESIDE | ShapeFlags.NOZWRITE | ShapeFlags.ONCE | ShapeFlags.NOCULL;
		if (!m_drawShapeSurface && m_drawShapeOutline) flags = flags | ShapeFlags.WIREFRAME;
		if (m_drawShapeSurface && !m_drawShapeOutline) flags = flags | ShapeFlags.NOOUTLINE;
		
		dbgShape = Shape.CreateSphere(c.PackToInt(), flags, GetOrigin(), m_queryRadius);
	}
	
}

enum TILW_EComparisonMode
{
	EQUAL_OR_LESS,
	EQUAL_OR_MORE
}

// Optionally, a bool that determines the receivers (global/local/faction) and notification color