[ComponentEditorProps(category: "GameScripted/", description: "Adds text to the map using polyline points as reference locations.")]
class TILW_MapTextComponentClass : ScriptComponentClass
{
}

class TILW_MapTextComponent : ScriptComponent
{
	protected SCR_MapEntity m_mapEntity;

	protected ref array<vector> m_points3D = new array<vector>();
	protected bool m_isClosed = true;

	protected MapItem m_textMarkerCenter;
	protected MapItem m_textMarkerStart;
	protected MapItem m_textMarkerEnd;
	
	// Visibility


[Attribute("", UIWidgets.Auto, "If defined, only display the text to given factions.", category: "Visibility")]
protected ref array<FactionKey> m_factionKeys;

[Attribute("1", UIWidgets.Auto, "Is the text visible before slotting or for spectators?", category: "Visibility")]
protected bool m_visibleForEmptyFaction;

	// Text Settings

	[Attribute("", UIWidgets.EditBox, "Text to display on the map.", category: "Text")]
	protected string m_linetext;

	[Attribute("255 255 255 255", UIWidgets.ColorPicker, "Text color.", category: "Text")]
	protected ref Color m_textColor;

	[Attribute("24", UIWidgets.Auto, "Text size.", params: "1 inf 0", category: "Text")]
	protected float m_fTextSize;

	[Attribute("0.5", UIWidgets.Auto, "Minimum text scale multiplier.", params: "0 inf 0.1", category: "Text")]
	protected float m_fTextMinScale;

	[Attribute("2.0", UIWidgets.Auto, "Maximum text scale multiplier.", params: "0 inf 0.1", category: "Text")]
	protected float m_fTextMaxScale;

	[Attribute("0", UIWidgets.Auto, "Text rotation angle.", params: "-360 360 1", category: "Text")]
	protected float m_fTextAngle;

	[Attribute("0", UIWidgets.CheckBox, "Bold text.", category: "Text")]
	protected bool m_bBoldText;

	[Attribute("0", UIWidgets.CheckBox, "Italic text.", category: "Text")]
	protected bool m_bItalicText;

	[Attribute("RobotoCondensed", UIWidgets.EditBox, "Font name.", category: "Text")]
	protected string m_sFont;

	[Attribute("1", UIWidgets.CheckBox, "Show text at center.", category: "Text")]
	protected bool m_bTextCenter;

	[Attribute("0", UIWidgets.CheckBox, "Show text at start.", category: "Text")]
	protected bool m_bTextStart;

	[Attribute("0", UIWidgets.CheckBox, "Show text at end.", category: "Text")]
	protected bool m_bTextEnd;

	void TILW_MapTextComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
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
		if (!m_mapEntity)
			m_mapEntity = SCR_MapEntity.GetMapInstance();

		if (!m_mapEntity)
			return;

		ScriptInvokerBase<MapConfigurationInvoker> onMapOpen = m_mapEntity.GetOnMapOpen();
		ScriptInvokerBase<MapConfigurationInvoker> onMapClose = m_mapEntity.GetOnMapClose();

		onMapOpen.Insert(CreateMapText);
		onMapClose.Insert(DeleteMapText);

		InitPoints();
	}

	protected void InitPoints()
	{
		m_points3D = {};

		PolylineShapeEntity polyline = PolylineShapeEntity.Cast(GetOwner());
		if (!polyline)
		{
			Print("TILW_MapTextComponent | Owner entity is not a PolylineShapeEntity.", LogLevel.WARNING);
			return;
		}

		if (polyline.GetPointCount() < 1)
		{
			Print("TILW_MapTextComponent | Polyline has no points.", LogLevel.WARNING);
			return;
		}

		polyline.GetPointsPositions(m_points3D);

		for (int i = 0; i < m_points3D.Count(); i++)
			m_points3D[i] = polyline.CoordToParent(m_points3D[i]);

		m_isClosed = polyline.IsClosed();
	}

protected void CreateMapText(MapConfiguration mapConfig)
{
	if (!IsVisible())
		return;

	CreateOrUpdateTextMarkers();
}

	protected void DeleteMapText(MapConfiguration mapConfig)
	{
		DeleteTextMarker(m_textMarkerCenter);
		DeleteTextMarker(m_textMarkerStart);
		DeleteTextMarker(m_textMarkerEnd);
	}

	protected void CreateOrUpdateTextMarkers()
	{
		if (!m_mapEntity)
			m_mapEntity = SCR_MapEntity.GetMapInstance();

		if (!m_mapEntity)
			return;

		if (m_linetext == string.Empty || m_linetext == "")
		{
			DeleteTextMarker(m_textMarkerCenter);
			DeleteTextMarker(m_textMarkerStart);
			DeleteTextMarker(m_textMarkerEnd);
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
			foreach (vector p : m_points3D)
			{
				centerX += p[0];
				centerY += p[2];
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

		if (m_bTextCenter)
		{
			if (!m_textMarkerCenter)
				m_textMarkerCenter = m_mapEntity.CreateCustomMapItem();

			SetupTextMarker(m_textMarkerCenter, centerX, centerY);
		}
		else
		{
			DeleteTextMarker(m_textMarkerCenter);
		}

		if (m_bTextStart)
		{
			if (!m_textMarkerStart)
				m_textMarkerStart = m_mapEntity.CreateCustomMapItem();

			SetupTextMarker(m_textMarkerStart, start[0], start[2]);
		}
		else
		{
			DeleteTextMarker(m_textMarkerStart);
		}

		if (m_bTextEnd)
		{
			if (!m_textMarkerEnd)
				m_textMarkerEnd = m_mapEntity.CreateCustomMapItem();

			SetupTextMarker(m_textMarkerEnd, end[0], end[2]);
		}
		else
		{
			DeleteTextMarker(m_textMarkerEnd);
		}
	}

	protected void SetupTextMarker(MapItem marker, float x, float y)
	{
		if (!marker)
			return;

		marker.SetPos(x, y);
		marker.SetVisible(true);
		marker.SetDisplayName(m_linetext);
		marker.SetImageDef("");

		MapDescriptorProps props = marker.GetProps();
		if (!props)
			return;

		props.SetFrontColor(Color.FromRGBA(0, 0, 0, 0));
		props.SetBackgroundColor(Color.FromRGBA(0, 0, 0, 0));
		props.SetOutlineColor(Color.FromRGBA(0, 0, 0, 0));

		props.SetIconVisible(false);
		props.SetImageDef("");
		props.SetIconSize(0.001, 0.001, 0.001);

		props.SetTextVisible(true);
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

	protected void DeleteTextMarker(out MapItem marker)
	{
		if (!marker)
			return;

		marker.SetVisible(false);
		marker.Recycle();
		marker = null;
	}
	
	
	protected bool IsVisible()
{
	SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
	if (!factionManager)
		return true;

	SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
	if (!playerController)
		return true;

	SCR_PlayerFactionAffiliationComponent playerFactionAffiliationComponent;
	playerFactionAffiliationComponent = SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));

	if (!playerFactionAffiliationComponent)
		return true;

	Faction faction = playerFactionAffiliationComponent.GetAffiliatedFaction();

	FactionKey fkey = "";
	if (faction)
		fkey = faction.GetFactionKey();

	if (fkey == "")
		return m_visibleForEmptyFaction;

	return m_factionKeys.IsEmpty() || m_factionKeys.Contains(fkey);
}
}