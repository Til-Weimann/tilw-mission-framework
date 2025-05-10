[EntityEditorProps(category: "game/", description: "Allows you to set the entities name.")]
class TILW_SetNameComponentClass: ScriptComponentClass
{
}

class TILW_SetNameComponent: ScriptComponent
{
	[Attribute("", UIWidgets.Auto, "Set the name of the entity")]
	protected string m_setName;
	
	override void OnPostInit(IEntity owner)
	{
		if (Replication.IsServer() && m_setName != "")
			owner.SetName(m_setName);
	}
}