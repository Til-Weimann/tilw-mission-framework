modded class SCR_CharacterControllerComponent : CharacterControllerComponent
{
	protected string m_fwCount = "";

	void SCR_CharacterControllerComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		if (!Replication.IsServer())
			return;
		if (m_fwCount == "")
			GetGame().GetCallqueue().CallLater(AddLife, 250, false); // 5 delay is enough for player check to work in mptest
	}
	override void OnDeath(IEntity instigatorEntity, notnull Instigator instigator)
	{
		super.OnDeath(instigatorEntity, instigator);
		if (!Replication.IsServer())
			return;
		if (m_fwCount != "")
			GetGame().GetCallqueue().CallLater(AddDeath, 250, false); // symmetry to avoid life removal attempt before addition
		TILW_TriggerSystem ts = TILW_TriggerSystem.GetInstance();
		if (ts)
			ts.RemoveCharacter(GetCharacter());
	}
	void ~SCR_CharacterControllerComponent()
	{
		if (!Replication.IsServer())
			return;
		if (m_fwCount != "")
			RemoveLife();
	}

	protected void AddLife()
	{
		// Is framework there?
		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		if (!fw)
			return;
		// Is character there?
		SCR_ChimeraCharacter cc = GetCharacter();
		if (!cc)
			return;
		// Is not playable?
		PS_PlayableComponent pc = PS_PlayableComponent.Cast(cc.FindComponent(PS_PlayableComponent));
		if (!pc || pc.GetPlayable())
			return;
		// Is not a player?
		if (EntityUtils.IsPlayer(cc))
			return;
		// Does it have a faction?
		string fkey = cc.GetFactionKey();
		if (!fkey || fkey == "")
			return;

		fw.m_factionAILifes.Set(fkey, fw.m_factionAILifes.Get(fkey) + 1);
		m_fwCount = fkey;
		fw.ScheduleRecountAI();
	}

	protected void RemoveLife()
	{
		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		if (!fw)
			return;
		fw.m_factionAILifes.Set(m_fwCount, fw.m_factionAILifes.Get(m_fwCount) - 1);
		m_fwCount = "";
		fw.ScheduleRecountAI();
	}

	protected void AddDeath()
	{
		TILW_MissionFrameworkEntity fw = TILW_MissionFrameworkEntity.GetInstance();
		if (!fw)
			return;
		fw.m_factionAIDeaths.Set(m_fwCount, fw.m_factionAIDeaths.Get(m_fwCount) + 1);
		m_fwCount = "";
		fw.ScheduleRecountAI();
	}
}
