[EntityEditorProps(category: "GameScripted/". description: "Entity which spawns prefab groups at runtime and assigns random waypoints to simulate civilians.")]
class CUN_CivilianPopulatorEntityClass: GenericEntityClass
{
}

class CUN_CivilianPopulatorEntity : GenericEntity
{
	[Attribute("", uiwidget: UIWidgets.Auto, desc: "Groups you want to spawn at runtime.")]
	protected array<ResourceName> m_prefabs;
}