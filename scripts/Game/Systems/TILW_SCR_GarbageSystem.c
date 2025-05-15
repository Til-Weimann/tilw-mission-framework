//! Script entry for garbage system modding
modded class SCR_GarbageSystem : GarbageSystem
{
	override protected float OnInsertRequested(IEntity entity, float lifetime)
	{
		// Prevent insertion of entities with NoGarbageComponent
		if (entity && entity.FindComponent(TILW_NoGarbageComponent))
			return -1;

		return super.OnInsertRequested(entity, lifetime);
	}
}