[ComponentEditorProps(category: "GameScripted/", description: "When added to a polyline, this component can draw various shapes onto the map.")]
class TILW_MapShapeComponentClass : ScriptComponentClass
{
}

class TILW_MapShapeComponent : ScriptComponent
{
	protected SCR_MapEntity m_mapEntity;
	
	protected ref array<vector> m_points3D = new array<vector>();
	
	protected ref array<float> m_points2D = new array<float>();
	protected ref array<float> m_points2D_1 = new array<float>();
	protected ref array<float> m_points2D_2 = new array<float>();
	
	protected bool m_isClosed = true;
	
	// Visibility
	
	[Attribute("", UIWidgets.Auto, "If defined, only display the shape to given factions.", category: "Visibility")]
	protected ref array<FactionKey> m_factionKeys;
	
	[Attribute("1", UIWidgets.Auto, "Is the shape visible before slotting or for spectators?", category: "Visibility")]
	protected bool m_visibleForEmptyFaction;
	
	// Area
	
	[Attribute("1", UIWidgets.Auto, "Fill the area", category: "Area")]
	protected bool m_drawArea;
	
	[Attribute("0 0 0 0.75", UIWidgets.ColorPicker, desc: "Color of the shape, you can also set transparency.", category: "Area")]
	protected ref Color m_color;
	
	[Attribute("0", UIWidgets.Auto, "Invert the area so it encompasses the polyline.", category: "Area")]
	protected bool m_invert;
	
	// Line
	
	[Attribute("0", UIWidgets.Auto, "Draw a line following the polyline", category: "Line")]
	protected bool m_drawLine;

	[Attribute("0 0 0 1", UIWidgets.ColorPicker, "Color of the line.", category: "Line")]
	protected ref Color m_lineColor;

	[Attribute("5", UIWidgets.Auto, "Width of the line in meters, on-screen width depends on zoom level.", params: "1 inf 0", category: "Line")]
	protected int m_lineWidth;
	
	// Text
	
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
	
	[Attribute("0", UIWidgets.EditBox, "Text placement: 0=Center, 1=Start, 2=End, 3=Start+End", "0 3 1", category: "Text")]
	int m_iTextPlacement;
	
	[Attribute("0 0 0 1", UIWidgets.ColorPicker, desc: "Text color", category: "Text")]
	protected ref Color m_textColor;
	
	[Attribute("RobotoCondensed-Bold", UIWidgets.EditBox, desc: "Font name", category: "Text")]
	string m_sFont;
	
	
	protected bool IsVisible()
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return true;
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
			return true;
		
		SCR_PlayerFactionAffiliationComponent playerFactionAffiliationComponent = SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (!playerFactionAffiliationComponent)
			return true;
		
		Faction faction = playerFactionAffiliationComponent.GetAffiliatedFaction();
		FactionKey fkey = "";
		if (faction)
			fkey = faction.GetFactionKey();
		
		if (fkey == "")
			return m_visibleForEmptyFaction;
		
		return (m_factionKeys.IsEmpty() || m_factionKeys.Contains(fkey));
	}
	
	void TILW_MapShapeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		SetEventMask(ent, EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!GetGame().InPlayMode())
			return;
		m_mapEntity = SCR_MapEntity.GetMapInstance();
		GetGame().GetCallqueue().Call(Init);
	}
	
	protected void Init()
	{
		ScriptInvokerBase<MapConfigurationInvoker> onMapOpen = m_mapEntity.GetOnMapOpen();
		ScriptInvokerBase<MapConfigurationInvoker> onMapClose = m_mapEntity.GetOnMapClose();
		
		onMapOpen.Insert(CreateMapWidget);
		onMapClose.Insert(DeleteMapWidget);
		
		InitPoints();
	}
	
	
	
	void SetFactions(array<FactionKey> factions)
	{
		if (SCR_ArrayHelperT<FactionKey>.AreEqual(factions, m_factionKeys))
			return;
		m_factionKeys = factions;
		
		Rpc(RpcDo_SetFactions, factions);
		
		if (RplSession.Mode() != RplMode.Dedicated)
			Recompute();
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetFactions(array<FactionKey> factions)
	{
		m_factionKeys = factions;
		Recompute();
	}
	
	void SetPoints3D(array<vector> points3D, bool closed = true)
	{
		if (points3D.Count() < 2 + m_drawArea)
		{
			Print("TILW_MapShapeComponent | SetPoints3D did not receive enough points!", LogLevel.ERROR);
			return;
		}
		
		Rpc(RpcDo_SetPoints3D, points3D, closed);
		
		m_points3D = points3D;
		m_isClosed = closed;
		
		if (RplSession.Mode() != RplMode.Dedicated)
			OnPoints3DChange();
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetPoints3D(array<vector> points3D, bool closed)
	{
		m_points3D = points3D;
		m_isClosed = closed;
		OnPoints3DChange();
	}
	
	
	protected void InitPoints()
	{
		m_points3D = {};
		
		PolylineShapeEntity pse = PolylineShapeEntity.Cast(GetOwner());
		if (!pse) {
			Print("TILW_MapShapeComponent | Owner entity (" + GetOwner() + ") is not a polyline!", LogLevel.WARNING);
			return;
		}
		if (pse.GetPointCount() < 2 + m_drawArea) {
			Print("TILW_MapShapeComponent | Owner entity (" + GetOwner() + ") does not have enough points!", LogLevel.WARNING);
			return;
		}
		pse.GetPointsPositions(m_points3D);
		
		for (int i = 0; i < m_points3D.Count(); i++)
			m_points3D[i] = pse.CoordToParent(m_points3D[i]);
		
		m_isClosed = pse.IsClosed();
		
		OnPoints3DChange(false);
	}
		
		
	protected void OnPoints3DChange(bool show = true)
	{
		if (!m_mapEntity)
			m_mapEntity = SCR_MapEntity.GetMapInstance();
		
		m_points2D = {};
		m_points2D_1 = {};
		m_points2D_2 = {};
		
		SCR_Math2D.Get2DPolygon(m_points3D, m_points2D);
		
		if (m_invert)
			InvertPolygon();
		else
			m_points2D_1 = m_points2D;
		
		if (RplSession.Mode() != RplMode.Dedicated && show)
			Recompute();
	}
	
	protected void UpdateLineWidth()
	{
		float zoom = m_mapEntity.GetCurrentZoom();
		m_drawLineCommand.m_fWidth = Math.Max(1, m_lineWidth * zoom);
	}
	
	protected void InvertPolygon()
	{
		vector cMin = m_mapEntity.Offset();
		vector cMax = m_mapEntity.Size() + m_mapEntity.Offset();
		
		vector v_bl = Vector(cMin[0], 0, cMin[2]);
		vector v_tr = Vector(cMax[0], 0, cMax[2]);
		
		vector v_tl = Vector(cMin[0], 0, cMax[2]);
		vector v_br = Vector(cMax[0], 0, cMin[2]);
		
		float min_bl, min_tr;
		int i_bl = -1;
		int i_tr = -1;
		
		for (int i = 0; i < m_points3D.Count(); i++)
		{
			vector v_cur = Vector(m_points3D[i][0], 0, m_points3D[i][2]);
			float cur_bl = vector.Distance(v_cur, v_bl);
			if (i_bl == -1 || cur_bl < min_bl)
			{
				i_bl = i;
				min_bl = cur_bl;
			}
			float cur_tr = vector.Distance(v_cur, v_tr);
			if (i_tr == -1 || cur_tr < min_tr)
			{
				i_tr = i;
				min_tr = cur_tr;
			}
		}
		
		array<vector> pg_1 = {};
		array<vector> pg_2 = {};
		
		bool inserting = false;
		for (int i = 0; i < m_points3D.Count(); i++)
		{
			if (i == i_bl)
				inserting = true;
			if (inserting)
				pg_1.Insert(m_points3D[i]);
			if (inserting && i == i_tr)
				break;
			if (i == m_points3D.Count() - 1)
				i = -1;
		}
		inserting = false;
		for (int i = 0; i < m_points3D.Count(); i++)
		{
			if (i == i_tr)
				inserting = true;
			if (inserting)
				pg_2.Insert(m_points3D[i]);
			if (inserting && i == i_bl)
			break;
			if (i == m_points3D.Count() - 1)
				i = -1;
		}
		
		bool cw = IsClockwise();
		
		pg_1.Insert(v_tr);
		if (cw)
			pg_1.Insert(v_tl);
		else
			pg_1.Insert(v_br);
		pg_1.Insert(v_bl);
		
		pg_2.Insert(v_bl);
		if (cw)
			pg_2.Insert(v_br);
		else
			pg_2.Insert(v_tl);
		pg_2.Insert(v_tr);
	
		SCR_Math2D.Get2DPolygon(pg_1, m_points2D_1);
		SCR_Math2D.Get2DPolygon(pg_2, m_points2D_2);
	}
	
	protected bool IsClockwise()
	{
		float sum = 0;
		for (int i = 0; i < m_points3D.Count(); i++)
		{
			vector p1 = m_points3D[i];
			vector p2 = m_points3D[(i + 1) % m_points3D.Count()];
			sum += (p2[0] - p1[0]) * (p2[2] + p1[2]);
		}
		return (sum > 0);
	}
	
	protected CanvasWidget m_wCanvasWidget;

	protected ref PolygonDrawCommand m_drawPolygon1 = new PolygonDrawCommand();
	protected ref PolygonDrawCommand m_drawPolygon2 = new PolygonDrawCommand();
	protected ref LineDrawCommand m_drawLineCommand = new LineDrawCommand();
	protected ref array<ref CanvasWidgetCommand> m_drawCommands = null;

	protected MapItem m_textMarkerStart;
	protected MapItem m_textMarkerEnd;
	
	protected vector m_previousPan;
	protected float m_previousZoom;
	
	protected void CreateMapWidget(MapConfiguration mapConfig)
	{
		if (!IsVisible())
			return;
		
		Widget mapFrame = m_mapEntity.GetMapMenuRoot().FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
		if (!mapFrame)
			mapFrame = m_mapEntity.GetMapMenuRoot();
		if (!mapFrame)
			 return;
		
		m_wCanvasWidget = CanvasWidget.Cast(GetGame().GetWorkspace().CreateWidgets("{B8793707B56B2F9F}UI/Map/PolyMapMarkerBase.layout", mapFrame));
		
		m_drawPolygon1.m_iColor = m_color.PackToInt();
		m_drawPolygon2.m_iColor = m_color.PackToInt();
		m_drawLineCommand.m_iColor = m_lineColor.PackToInt();
		UpdateLineWidth();
		
		if (!m_drawCommands)
		{
			m_drawCommands = {};
			
			if (m_drawArea)
			{
				m_drawCommands.Insert(m_drawPolygon1);
				if (m_invert)
					m_drawCommands.Insert(m_drawPolygon2);
			}
			
			if (m_drawLine)
				m_drawCommands.Insert(m_drawLineCommand);
		}
		
		m_wCanvasWidget.SetDrawCommands(m_drawCommands);
		SetEventMask(GetOwner(), EntityEvent.POSTFRAME);
		CreateOrUpdateTextMarker();
	}
	
	protected void DeleteMapWidget(MapConfiguration mapConfig)
	{
		m_drawCommands = null;
		ClearEventMask(GetOwner(), EntityEvent.POSTFRAME);

		if (m_textMarkerStart)
		{
			m_textMarkerStart.SetVisible(false);
			m_textMarkerStart.Recycle();
			m_textMarkerStart = null;
		}

		if (m_textMarkerEnd)
		{
			m_textMarkerEnd.SetVisible(false);
			m_textMarkerEnd.Recycle();
			m_textMarkerEnd = null;
		}
	}
	
	override void EOnPostFrame(IEntity owner, float timeSlice)
	{
		if (m_previousZoom != m_mapEntity.GetCurrentZoom())
		{
			UpdateLineWidth();
			m_previousZoom = m_mapEntity.GetCurrentZoom();
		}
		else if (m_previousPan == m_mapEntity.GetCurrentPan())
			return;
	
		m_previousPan = m_mapEntity.GetCurrentPan();
	
		Recompute();
	}
	
	protected void Recompute()
	{		
		if (m_drawLine)
		{
			m_drawLineCommand.m_Vertices = new array<float>();
			for (int i = 0; i < m_points2D.Count(); i += 2)
			{
				float screenX, screenY;
				m_mapEntity.WorldToScreen(m_points2D[i], m_points2D[i + 1], screenX, screenY, true);
				
				m_drawLineCommand.m_Vertices.Insert(screenX);
				m_drawLineCommand.m_Vertices.Insert(screenY);
			}
			if (m_isClosed && m_points2D.Count() > 4)
			{
				float screenX, screenY;
				m_mapEntity.WorldToScreen(m_points2D[0], m_points2D[1], screenX, screenY, true);
				m_drawLineCommand.m_Vertices.Insert(screenX);
				m_drawLineCommand.m_Vertices.Insert(screenY);
			}
		}
		
		if (!m_drawArea)
		{
			CreateOrUpdateTextMarker();
			return;
		}
		
		m_drawPolygon1.m_Vertices = new array<float>();
		for (int i = 0; i < m_points2D_1.Count(); i += 2)
		{
			float screenX, screenY;
			m_mapEntity.WorldToScreen(m_points2D_1[i], m_points2D_1[i + 1], screenX, screenY, true);
			
			m_drawPolygon1.m_Vertices.Insert(screenX);
			m_drawPolygon1.m_Vertices.Insert(screenY);
		}
		
		if (!m_invert)
		{
			CreateOrUpdateTextMarker();
			return;
		}
		
		m_drawPolygon2.m_Vertices = new array<float>();
		for (int i = 0; i < m_points2D_2.Count(); i += 2)
		{
			float screenX, screenY;
			m_mapEntity.WorldToScreen(m_points2D_2[i], m_points2D_2[i + 1], screenX, screenY, true);
			
			m_drawPolygon2.m_Vertices.Insert(screenX);
			m_drawPolygon2.m_Vertices.Insert(screenY);
		}

		CreateOrUpdateTextMarker();
	}

	protected void SetupTextMarker(MapItem marker, float x, float y)
	{
		if (!marker)
			return;

		marker.SetPos(x, y);
		marker.SetVisible(true);
		marker.SetDisplayName(m_linetext);

		MapDescriptorProps props = marker.GetProps();
		props.SetFrontColor(Color.FromRGBA(0, 0, 0, 0));
		props.SetTextSize(m_fTextSize, m_fTextMinScale * m_fTextSize, m_fTextMaxScale * m_fTextSize);

		if (m_bBoldText)
			props.SetTextBold();

		if (m_bItalicText)
			props.SetTextItalic();

		props.SetTextColor(m_textColor);
		// Remove this line if your build does not support it:
		props.SetFont(m_sFont);
		props.SetTextAngle(m_fTextAngle);
		props.Activate(true);
		marker.SetProps(props);
	}

	protected void CreateOrUpdateTextMarker()
	{
		if (!m_mapEntity)
			return;

		if (m_linetext == string.Empty || m_linetext == "")
		{
			if (m_textMarkerStart)
			{
				m_textMarkerStart.SetVisible(false);
				m_textMarkerStart.Recycle();
				m_textMarkerStart = null;
			}

			if (m_textMarkerEnd)
			{
				m_textMarkerEnd.SetVisible(false);
				m_textMarkerEnd.Recycle();
				m_textMarkerEnd = null;
			}
			return;
		}

		if (m_points3D.IsEmpty())
			return;

		vector start = m_points3D[0];
		vector end = m_points3D[m_points3D.Count() - 1];

		float centerX = 0;
		float centerY = 0;

		if (m_isClosed)
		{
			foreach (vector textloc : m_points3D)
			{
				centerX += textloc[0];
				centerY += textloc[2];
			}

			centerX = centerX / m_points3D.Count();
			centerY = centerY / m_points3D.Count();
		}
		else
		{
			int mid = m_points3D.Count() / 2;
			centerX = m_points3D[mid][0];
			centerY = m_points3D[mid][2];
		}

		switch (m_iTextPlacement)
		{
			case 1: // Start
			{
				if (!m_textMarkerStart)
					m_textMarkerStart = m_mapEntity.CreateCustomMapItem();

				SetupTextMarker(m_textMarkerStart, start[0], start[2]);

				if (m_textMarkerEnd)
				{
					m_textMarkerEnd.SetVisible(false);
					m_textMarkerEnd.Recycle();
					m_textMarkerEnd = null;
				}
				break;
			}

			case 2: // End
			{
				if (!m_textMarkerStart)
					m_textMarkerStart = m_mapEntity.CreateCustomMapItem();

				SetupTextMarker(m_textMarkerStart, end[0], end[2]);

				if (m_textMarkerEnd)
				{
					m_textMarkerEnd.SetVisible(false);
					m_textMarkerEnd.Recycle();
					m_textMarkerEnd = null;
				}
				break;
			}

			case 3: // Start+End
			{
				if (!m_textMarkerStart)
					m_textMarkerStart = m_mapEntity.CreateCustomMapItem();
				if (!m_textMarkerEnd)
					m_textMarkerEnd = m_mapEntity.CreateCustomMapItem();

				SetupTextMarker(m_textMarkerStart, start[0], start[2]);
				SetupTextMarker(m_textMarkerEnd, end[0], end[2]);
				break;
			}

			default: // Center
			{
				if (!m_textMarkerStart)
					m_textMarkerStart = m_mapEntity.CreateCustomMapItem();

				SetupTextMarker(m_textMarkerStart, centerX, centerY);

				if (m_textMarkerEnd)
				{
					m_textMarkerEnd.SetVisible(false);
					m_textMarkerEnd.Recycle();
					m_textMarkerEnd = null;
				}
				break;
			}
		}
	}
	
	
	override bool RplSave(ScriptBitWriter writer)
	{
		int factionKeysCount = m_factionKeys.Count();
		writer.WriteInt(factionKeysCount);
		for (int i = 0; i < factionKeysCount; i++)
		{
			writer.WriteString(m_factionKeys[i]);
		}
		
		int pointsCount = m_points3D.Count();
		writer.WriteInt(pointsCount);
		for (int i = 0; i < pointsCount; i++)
		{
			writer.WriteVector(m_points3D[i]);
		}
		
		writer.WriteBool(m_isClosed);
		
		return true;
	}
	
	override bool RplLoad(ScriptBitReader reader)
	{
		array<FactionKey> factionKeys = {};
		int factionKeysCount;
		reader.ReadInt(factionKeysCount);
		for (int i = 0; i < factionKeysCount; i++)
		{
			string factionKey;
			reader.ReadString(factionKey);
			factionKeys.Insert(factionKey);
		}
		
		array<vector> points = {};
		int pointsCount;
		reader.ReadInt(pointsCount);
		for (int i = 0; i < pointsCount; i++)
		{
			vector point;
			reader.ReadVector(point);
			points.Insert(point);
		}
		
		reader.ReadBool(m_isClosed);
		
		m_factionKeys = factionKeys;
		m_points3D = points;
		OnPoints3DChange();
		
		return true;
	}
}