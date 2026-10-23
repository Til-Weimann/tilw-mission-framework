[EntityEditorProps(
    category: "GameScripted/",
    description: "A spawn location used by the civilian populator."
)]
class CN_CivilianSpawnerTargetEntityClass : GenericEntityClass
{
}

class CN_CivilianSpawnerTargetEntity : GenericEntity
{
	[Attribute("0", uiwidget: UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(CN_CivilianSpawnerTargetType) , desc: "Type of target this is.", category: "Civilian Spawner Target")]
	CN_CivilianSpawnerTargetType m_targetType;
	
}

enum CN_CivilianSpawnerTargetType
{
	SPAWNER,
	DESTINATION,
	FLEEPOINT
}