[EntityEditorProps(category: "game/", description: "Regenerates navmesh around entity + children after initialization.")]
class TILW_NavmeshRegenComponentClass: ScriptComponentClass
{
}

class TILW_NavmeshRegenComponent: ScriptComponent
{
	override void OnPostInit(IEntity owner)
	{
		GetGame().GetCallqueue().Call(RegenEntity, owner);
		
		return;
		
		VolumeEntity ve = VolumeEntity.Cast(owner);
		if (!ve)
			GetGame().GetCallqueue().Call(RegenEntity, owner);
		else
			GetGame().GetCallqueue().Call(RegenVolume, ve);
	}
	
	protected static void RegenEntity(IEntity owner)
	{
		AIWorld aiw = GetGame().GetAIWorld();
		if (!aiw)
			return;
		SCR_AIWorld scr_aiw = SCR_AIWorld.Cast(aiw);
		if (!scr_aiw)
			return;
		scr_aiw.RequestNavmeshRebuildEntity(owner);
	}
	
	protected static void RegenVolume(VolumeEntity ve)
	{	
		array<vector> points3D = new array<vector>();
		array<float> points2D = new array<float>();
		
		AIWorld aiw = GetGame().GetAIWorld();
		if (!aiw)
			return;
		SCR_AIWorld scr_aiw = SCR_AIWorld.Cast(aiw);
		if (!scr_aiw)
			return;
	}
}