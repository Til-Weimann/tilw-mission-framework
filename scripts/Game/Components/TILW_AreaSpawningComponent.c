[ComponentEditorProps(category: "GameScripted/", description: "Area Spawning component")]
class TILW_AreaSpawningComponentClass: ScriptComponentClass
{
}

class TILW_AreaSpawningComponent : ScriptComponent
{
	
	[Attribute("", UIWidgets.Auto, "Prefabs to be spawned at random locations within the area \nThe area is defined by the components owner entity, which must be a polyline", "et")]
	protected ref array<ResourceName> m_prefabs;
	
	protected ref array<float> m_points2D = new array<float>();
	protected ref array<vector> m_points3D = new array<vector>();
	
	override void OnPostInit(IEntity owner)
	{
		if (Replication.IsServer())
			GetGame().GetCallqueue().Call(SpawnPrefabs, owner);
	}
	
	protected void SpawnPrefabs(IEntity owner)
	{
		PolylineShapeEntity pse = PolylineShapeEntity.Cast(owner);
		if (!pse) {
			Print("TILW_AreaSpawningComponent | Owner entity (" + owner + ") is not a polyline!", LogLevel.ERROR);
			return;
		}
		if (pse.GetPointCount() < 3) {
			Print("TILW_AreaSpawningComponent | Owner entity (" + owner + ") does not have enough points!", LogLevel.ERROR);
			return;
		}
		pse.GetPointsPositions(m_points3D);
		for (int i = 0; i < m_points3D.Count(); i++) m_points3D[i] = pse.CoordToParent(m_points3D[i]);
		SCR_Math2D.Get2DPolygon(m_points3D, m_points2D);
		
		float x, y;
		EntitySpawnParams params = new EntitySpawnParams();
		
		foreach (ResourceName r : m_prefabs)
		{
			SCR_Math2D.GetRandomPointInPolygon(m_points2D, x, y);
			params.Transform[3] = Vector(x, SCR_TerrainHelper.GetTerrainY(Vector(x, 0, y), null, true), y);
			IEntity e = GetGame().SpawnEntityPrefab(Resource.Load(r), GetGame().GetWorld(), params);
		}
	}
	
}