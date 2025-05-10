[EntityEditorProps(category: "GameScripted/Triggers", description: "The TILW_FactionTriggerEntity is a custom trigger designed to be used with the TILW_MissionFrameworkEntity. \nIt compares the number of a factions characters to the total amount of characters.")]
class TILW_FactionTriggerEntityClass : TILW_BaseTriggerEntityClass
{
}

class TILW_FactionTriggerEntity : TILW_BaseTriggerEntity
{

	// FILTER SETTINGS

	[Attribute("0", UIWidgets.Auto, "If true, only the ratio between players should matter. AI characters are ignored completely.", category: "Trigger Filter")]
	protected bool m_playersOnly;

	[Attribute("", UIWidgets.ResourceAssignArray, "Any of these faction keys will be completely ignored by the trigger.", category: "Trigger Filter")]
	protected ref array<string> m_ignoredFactions;

	// CONDITION SETTINGS

	[Attribute("", UIWidgets.Auto, "Which faction can capture this trigger?", category: "Trigger Condition")]
	protected string m_ownerFactionKey;

	[Attribute("1", UIWidgets.Auto, "How high/low the factions character share has to be", category: "Trigger Condition", params: "0 1 0.01")]
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
			return (m_comparisonMode && ratio >= m_ratioThreshold);
	}
	
	override bool TotalFilter(SCR_ChimeraCharacter cc)
	{
		if (IsEntityDestroyed(cc))
			return false; // Not alive
		if (!IsWithinShape(cc.GetOrigin()))
			return false; // Not within shape
		if (!EntityUtils.IsPlayer(cc))
		{
			if (m_playersOnly)
				return false; // Only players allowed
			PS_PlayableComponent pc = PS_PlayableComponent.Cast(cc.FindComponent(PS_PlayableComponent));
			if (pc && pc.GetPlayable())
				return false; // Not slotted
		}
		FactionKey fkey = GetFactionKey(cc);
		if (fkey == "" || m_ignoredFactions.Contains(fkey))
			return false; // Spectator, or ignored faction
		return true;
	}
	
	override bool SpecialFilter(SCR_ChimeraCharacter cc)
	{
		FactionKey fkey = GetFactionKey(cc);
		if (GetFactionKey(cc) == m_ownerFactionKey)
			return true;
		return false;
	}
	
	override string GetStatusMessage(int status)
	{
		string factionName = GetGame().GetFactionManager().GetFactionByKey(m_ownerFactionKey).GetFactionName();
		return string.Format(super.GetStatusMessage(status), factionName);
	}
	
	// ACCESS POINTS

	void SetOwnerFaction(string key)
	{
		m_ownerFactionKey = key;
	}
	
}