modded class SCR_ChimeraCharacter : ChimeraCharacter
{
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		TILW_TriggerSystem ts = TILW_TriggerSystem.GetInstance();
		if (ts)
			ts.InsertCharacter(this);
	}

	void ~SCR_ChimeraCharacter()
	{
		TILW_TriggerSystem ts = TILW_TriggerSystem.GetInstance();
		if (ts)
			ts.RemoveCharacter(this);
	}
}