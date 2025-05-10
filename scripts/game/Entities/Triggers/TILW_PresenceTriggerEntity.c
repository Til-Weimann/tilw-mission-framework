[EntityEditorProps(category: "GameScripted/Triggers", description: "The TILW_PresenceTriggerEntity is a custom trigger designed to be used with the TILW_MissionFrameworkEntity.")]
class TILW_PresenceTriggerEntityClass : TILW_BaseTriggerEntityClass
{
}

class TILW_PresenceTriggerEntity : TILW_BaseTriggerEntity
{

	// FILTER SETTINGS

	[Attribute("", UIWidgets.Auto, "Names of the entities to look for. Can also find inventory items.", category: "Trigger Filter")]
	protected ref array<string> m_entityNames;

	[Attribute("0", UIWidgets.Auto, "If false, prevent dead or destroyed entities from being counted.", category: "Trigger Filter")]
	protected bool m_allowDestroyed;

	// CONDITION SETTINGS

	[Attribute("1", UIWidgets.Auto, "What percentage of the specified entities has to be present.", category: "Trigger Condition", params: "0 inf 1")]
	protected float m_ratioThreshold;


	// TRIGGER LOGIC

	override bool EvaluateCondition()
	{
		m_totalCount = m_entityNames.Count();
		
		if (m_totalCount == 0)
			return !m_comparisonMode;
		
		float ratio = m_specialCount / m_totalCount;
		
		if (!m_comparisonMode)
			return (ratio <= m_ratioThreshold);
		else
			return (ratio >= m_ratioThreshold);
	}
	
	override void CustomQuery()
	{
		m_specialCount = 0;
		
		foreach (string name : m_entityNames)
		{
			IEntity e = GetGame().GetWorld().FindEntityByName(name);
			if (!e)
				continue;
			float distance = vector.Distance(GetOrigin(), e.GetOrigin());
			if (distance > m_queryRadius)
				continue;
			if (!m_allowDestroyed && IsEntityDestroyed(e))
				continue;
			
			m_specialCount += 1;
		}
	}
	
	// ACCESS POINTS
	
	void SetEntityNames(array<string> entityNames)
	{
		m_entityNames = entityNames;
	}

}
