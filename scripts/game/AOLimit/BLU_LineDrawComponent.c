[ComponentEditorProps(category: "GameScripted/", description: "AO Limit component")]
class BLU_LineDrawComponentClass : ScriptComponentClass
{
}

class BLU_LineDrawComponent : ScriptComponent
{
	// Logic



	[RplProp(onRplName: "DrawAO"), Attribute("", UIWidgets.Auto, desc: "Factions affected by the AO limit (if empty, all factions)", category: "Logic")]
	protected ref array<string> m_factionKeys;




	// Visualization

	[Attribute("0", UIWidgets.ComboBox, "Who can view the AO limit (everyone, affected factions, noone)", enums: ParamEnumArray.FromEnum(TILW_EVisibilityMode), category: "Visualization")]
	protected TILW_EVisibilityMode m_visibility;

	[Attribute("0 0 0 1", UIWidgets.ColorPicker, "The default color of the drawn AO limit line.", category: "Visualization")]
	protected ref Color m_defaultColor;

	[Attribute("1", UIWidgets.Auto, "If this AO limit only affects certain factions, use the faction color of the first one instead.", category: "Visualization")]
	protected bool m_useFactionColor;

	[Attribute("3", UIWidgets.Auto, "Width of the AO limit line.", params: "1 inf 0", category: "Visualization")]
	protected int m_lineWidth;
	
	
	[Attribute("1", UIWidgets.Auto, "Links the First Polygon Point and Last Polygon Point Together", category: "Visualization")]
	protected bool m_closedpoly;
	
	[Attribute("25", UIWidgets.EditBox, desc: "text size", "1 1000 1", category: "Text")]
	float m_fTextSize;
	
	[Attribute("0.75", UIWidgets.EditBox, desc: "text minimum scale, multiplier of the size", params: "0.01 1", category: "Text")]
	float m_fTextMinScale;
	
	[Attribute("1.5", UIWidgets.EditBox, desc: "text maximum scale, multiplier of the size", params: "1 100", category: "Text")]
	float m_fTextMaxScale;
	
	[Attribute("", UIWidgets.Auto, "Text", category: "Text")]
	string m_linetext;
	
	[Attribute("0", UIWidgets.CheckBox, desc: "Use bold text", category: "Text")]
	bool m_bBoldText;
	
	[Attribute("0", UIWidgets.CheckBox, desc: "Use italic text", category: "Text")]
	bool m_bItalicText;

	[Attribute("0", UIWidgets.EditBox, desc: "text rotation angle", "-359.0 359.0 0.0", category: "Text")]
	float m_fTextAngle;



	[RplProp(onRplName: "OnPoints3DChange")]
	protected ref array<vector> m_points3D = new array<vector>();

	protected ref array<float> m_points2D = new array<float>();

	protected ref array<MapItem> m_markers = new array<MapItem>();



	protected TILW_AOLimitDisplay m_aoLimitDisplay;

	void BLU_LineDrawComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		SetEventMask(ent, EntityEvent.INIT);
	}

	protected override void EOnInit(IEntity owner)
	{
		PolylineShapeEntity pse = PolylineShapeEntity.Cast(GetOwner());
		if (!pse) {
			Print("BLU_LineDrawComponent | Owner entity (" + GetOwner() + ") is not a polyline!", LogLevel.WARNING);
			return;
		}
		if (pse.GetPointCount() < 3) {
			Print("BLU_LineDrawComponent | Owner entity (" + GetOwner() + ") does not have enough points!", LogLevel.WARNING);
			return;
		}

		pse.GetPointsPositions(m_points3D);
		for (int i = 0; i < m_points3D.Count(); i++)
			m_points3D[i] = pse.CoordToParent(m_points3D[i]);

		SCR_Math2D.Get2DPolygon(m_points3D, m_points2D);

		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		SetEventMask(GetOwner(), EntityEvent.FIXEDFRAME);
		DrawAO();

		if (m_visibility == TILW_EVisibilityMode.FACTION)
		{
			PS_PlayableManager pm = PS_PlayableManager.GetInstance();
			pm.GetOnFactionChange().Insert(FactionChange);
		}
	}

	void FactionChange(int playerId, FactionKey factionKey, FactionKey factionKeyOld)
	{
		if (m_factionKeys.IsEmpty() || m_visibility != TILW_EVisibilityMode.FACTION)
			return;
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc || pc.GetPlayerId() != playerId)
			return;
		if (factionKey == factionKeyOld || m_factionKeys.Contains(factionKey) == m_factionKeys.Contains(factionKeyOld))
			return;
		DrawAO();
	}


	protected IEntity m_controlledEntity;
	protected bool m_wasEverInsideAO = false;


	
	void SetFactions(array<string> factions)
	{
		m_factionKeys = factions;
		Replication.BumpMe();

		if (RplSession.Mode() != RplMode.Dedicated)
			DrawAO();
	}

	void SetPoints(array<vector> points)
	{
		if (points.Count() < 3) {
			Print("TILW_AOLimitComponent | not enough points!", LogLevel.ERROR);
			return;
		}

		m_points3D = points;
		Replication.BumpMe();

		if (RplSession.Mode() != RplMode.Dedicated)
			OnPoints3DChange();
	}

	protected void OnPoints3DChange()
	{
		SCR_Math2D.Get2DPolygon(m_points3D, m_points2D);

		EntityEvent mask = GetEventMask();
		if (mask != EntityEvent.FIXEDFRAME)
			SetEventMask(GetOwner(), EntityEvent.FIXEDFRAME);

		DrawAO();
	}

	protected void DrawAO()
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		foreach (MapItem marker : m_markers)
		{
			marker.SetVisible(false);
			marker.Recycle();
		}
		m_markers.Clear();

		switch (m_visibility)
		{
			case TILW_EVisibilityMode.ALL:
				break;
			case TILW_EVisibilityMode.FACTION:
				PS_PlayableManager pm = PS_PlayableManager.GetInstance();
				PlayerController pc = GetGame().GetPlayerController();
				if (pm && pc && m_factionKeys.Contains(pm.GetPlayerFactionKey(pc.GetPlayerId())))
					break;
				else
					return;
			case TILW_EVisibilityMode.NONE:
				return;
			default:
				break;
		}

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity)
			return;
		MapItem lastItem;
		MapItem firstItem;

		Color c = m_defaultColor;
		if (m_useFactionColor && !m_factionKeys.IsEmpty() && m_factionKeys[0] != "")
			c = GetGame().GetFactionManager().GetFactionByKey(m_factionKeys[0]).GetFactionColor();

		foreach (vector point : m_points3D)
		{
			MapItem item = mapEntity.CreateCustomMapItem();
			item.SetPos(point[0], point[2]);
			item.SetVisible(true);
			m_markers.Insert(item);

			MapDescriptorProps props = item.GetProps();
			props.SetFrontColor(Color.FromRGBA(0, 0, 0, 0));
			props.Activate(true);
			item.SetProps(props);

			
			if (lastItem)
			{
				MapLink link = item.LinkTo(lastItem);
				MapLinkProps linkProps = link.GetMapLinkProps();

				linkProps.SetLineColor(c);
				linkProps.SetLineWidth(m_lineWidth);
			}
			

			if (!firstItem)
				firstItem = item;
			lastItem = item;
		}
		
		
		float textx = 0;
		float texty = 0;

		if (m_closedpoly)
		{
			MapLink link = lastItem.LinkTo(firstItem);
			MapLinkProps linkProps = link.GetMapLinkProps();
			linkProps.SetLineColor(c);
			linkProps.SetLineWidth(m_lineWidth);
			
			foreach (vector textloc:m_points3D)
			{
				textx = textx + textloc[0];
				texty = texty + textloc[2];
			}
			
			textx = textx/(m_points3D.Count());
			texty = texty/(m_points3D.Count());
			
			
			
			MapItem textmap = mapEntity.CreateCustomMapItem();
			textmap.SetPos(textx,texty);
			textmap.SetVisible(true);
			m_markers.Insert(textmap);
			textmap.SetDisplayName(m_linetext);
			
			MapDescriptorProps props = textmap.GetProps();
			props.SetFrontColor(Color.FromRGBA(0, 0, 0, 0));
			props.SetTextSize(m_fTextSize, m_fTextMinScale * m_fTextSize, m_fTextMaxScale * m_fTextSize);
			if (m_bBoldText)
				props.SetTextBold();
		
			if (m_bItalicText)
				props.SetTextItalic();
			
			props.SetTextColor(c);
			props.SetTextAngle(m_fTextAngle);
			props.Activate(true);
			textmap.SetProps(props);
			}
		else
		{
			
			MapDescriptorProps props = firstItem.GetProps();
			props.SetTextSize(m_fTextSize, m_fTextMinScale * m_fTextSize, m_fTextMaxScale * m_fTextSize);
			if (m_bBoldText)
				props.SetTextBold();
		
			if (m_bItalicText)
				props.SetTextItalic();
			
			props.SetTextColor(c);
			props.SetTextAngle(m_fTextAngle);
			props.Activate(true);
			
			firstItem.SetDisplayName(m_linetext);
			lastItem.SetDisplayName(m_linetext);
			firstItem.SetProps(props);
			lastItem.SetProps(props);
		
		}


		
	}
}