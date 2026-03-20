[EntityEditorProps(
    category: "GameScripted/",
    description: "Deletes entities in a certain radius. \n- Use the Prefab Filter Picker tool to build a filter list.\n- Right-click the filter list to copy/paste between entities.\n- Ctrl+Click prefabs in the picker tool to add/remove from the filter list."
)]
class PK_EntityDeleterClass : GenericEntityClass
{
}
//------------------------------------------------------------------------------------------------
class PK_EntityDeleter : GenericEntity
{
	// Modified version of SCR_PrefabDeleter
	// Adds support for prefab filtering and fixes the self deletion and mid-query deletion bugs
	
	[Attribute("www.youtube.com/watch?v=D-Bb66hJnqE", UIWidgets.Object, 
	desc: "INSTRUCTIONS:\n\n- Use the Prefab Picker tool to build a prefab list.\n- Right-click the prefab list to copy/paste between entities.\n- Ctrl+Click prefabs in the picker tool to add/remove from the filter list.", 
	category: "Entity Deleter")]
	protected string m_HowToUsePrefabPickerTool;
	
	[Attribute(defvalue: "20", uiwidget: UIWidgets.Slider, desc: "Radius in which entities are deleted.\nEntities with the EntityProtectorComponent are ignored, so are entities with hierarchy parents that have it.", "0 1000 1", category: "Entity Deleter")]
	protected float m_fRadius;

	[Attribute("", UIWidgets.ResourceAssignArray, desc: "What kinds of entities should (or should not) be deleted. If empty, all entities are deleted. Recognizes inheritance.", category: "Entity Deleter", params: "et")]
	protected ref array<ResourceName> m_prefabFilterList;
	
	[Attribute("0", UIWidgets.Auto, desc: "If enabled, prefabs in the filter list will be excluded from deletion. Otherwise, the filter will limit deletion to the listed prefabs.", category: "Entity Deleter")]
	protected bool m_excludePrefabsInList;
	
	[Attribute("0", UIWidgets.Auto, desc: "If enabled, only entities with VObjects (e. g. mesh or particle) are deleted, not purely logical entities.", category: "Entity Deleter")]
	protected bool m_bVObjectsOnly;
	
	[Attribute("1", UIWidgets.Auto, desc: "Whether to show a preview of the deletion radius in the World Editor.", category: "Entity Deleter")]
	protected bool m_bPreviewShape;
	
	protected ref array<IEntity> m_aEntitiesToDelete = {};

	
	void PK_EntityDeleter(IEntitySource src, IEntity parent)
	{
		if (!GetGame().InPlayMode())
			return;

		SetEventMask(EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!Replication.IsServer())
			return;
		GetGame().GetCallqueue().Call(PerformDeletion);
	}
	
	protected void PerformDeletion()
	{
		EQueryEntitiesFlags flags = EQueryEntitiesFlags.ALL;
		if (m_bVObjectsOnly)
			flags = EQueryEntitiesFlags.WITH_OBJECT;
		GetWorld().QueryEntitiesBySphere(GetOrigin(), m_fRadius, AddEntity, null, flags);
		
		foreach (IEntity e : m_aEntitiesToDelete)
			SCR_EntityHelper.DeleteEntityAndChildren(e);
		
		SCR_EntityHelper.DeleteEntityAndChildren(this);
	}
	
	private bool AddEntity(IEntity e)
	{
		if (e.GetParent() || e.FindComponent(PK_EntityProtectorComponent) || e.IsInherited(PK_EntityDeleter) || e.IsInherited(SCR_PrefabDeleterEntity))
			return true;
		
		if (!m_prefabFilterList.IsEmpty() && IsInPrefabList(e) == m_excludePrefabsInList)
			return true;
		
		m_aEntitiesToDelete.Insert(e);
		return true;
	}
	
	protected bool IsInPrefabList(IEntity e)
	{
		EntityPrefabData prefabData = e.GetPrefabData();
		if (!prefabData)
			return false;
		BaseContainer container = prefabData.GetPrefab();
		while (container)
		{
			if (m_prefabFilterList.Contains(container.GetResourceName()))
				return true;
			container = container.GetAncestor();
		}
		return false;
	}

#ifdef WORKBENCH

	override int _WB_GetAfterWorldUpdateSpecs(IEntitySource src)
	{
		return EEntityFrameUpdateSpecs.CALL_ALWAYS;
	}

	//------------------------------------------------------------------------------------------------
	override void _WB_AfterWorldUpdate(float timeSlice)
	{
		super._WB_AfterWorldUpdate(timeSlice);
		Shape debugShape = null;
		if (m_bPreviewShape)
			Shape radiusShape = Shape.CreateSphere(COLOR_YELLOW, ShapeFlags.WIREFRAME | ShapeFlags.ONCE, GetOrigin(), m_fRadius);
	}

#endif

}