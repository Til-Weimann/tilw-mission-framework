[EntityEditorProps(category: "GameScripted/Triggers", description: "The ExtractionTriggerEntity is a custom trigger designed to be used with the TILW_MissionFrameworkEntity. \nIt compares the number of local faction players to the number of global alive faction players.")]
class TILW_ExtractionTriggerEntityClass : TILW_BaseTriggerEntityClass
{
}

class TILW_ExtractionTriggerEntity : TILW_BaseTriggerEntity
{

	// FILTER SETTINGS

	// CONDITION SETTINGS
	
	[Attribute("1", UIWidgets.Auto, "If true, only the ratio between players should matter. AI characters are ignored completely.", category: "Trigger Filter")]
	protected bool m_playersOnly;

	[Attribute("", UIWidgets.Auto, "Which faction is being examined?", category: "Trigger Condition")]
	protected string m_factionKey;

	[Attribute("1", UIWidgets.Auto, "How high/low the ratio of local to global faction players needs to be.\nFor example, if the ratio is 0.9, 90% of all the factions alive players need to be within the radius for the flag to be set.", category: "Trigger Condition", params: "0 1 0.01")]
	protected float m_ratioThreshold;


	// TRIGGER LOGIC

	override bool EvaluateCondition()
	{
		if (m_totalCount == 0)
			return !m_comparisonMode;
		float ratio = m_specialCount / m_totalCount;
		if (!m_comparisonMode)
			return (ratio <= m_ratioThreshold);
		else
			return (ratio >= m_ratioThreshold);
	}
	
	override bool TotalFilter(SCR_ChimeraCharacter cc)
	{
		if (IsEntityDestroyed(cc))
			return false; // Not alive
		if (!EntityUtils.IsPlayer(cc))
		{
			if (m_playersOnly)
				return false; // Only players allowed
			PS_PlayableComponent pc = PS_PlayableComponent.Cast(cc.FindComponent(PS_PlayableComponent));
			if (pc && pc.GetPlayable())
				return false; // Not slotted
		}
		return true;
	}
	
	override bool SpecialFilter(SCR_ChimeraCharacter cc)
	{
		if (!IsWithinShape(cc.GetOrigin()))
			return false; // Not within shape
		FactionKey fkey = GetFactionKey(cc);
		if (GetFactionKey(cc) == m_factionKey)
			return true;
		return false;
	}

	override string GetStatusMessage(int status)
	{
		string factionName = GetGame().GetFactionManager().GetFactionByKey(m_factionKey).GetFactionName();
		return string.Format(super.GetStatusMessage(status), factionName);
	}
	
	// ACCESS POINTS
	
	void SetFaction(string key)
	{
		m_factionKey = key;
	}
}
