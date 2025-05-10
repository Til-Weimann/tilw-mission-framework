[ComponentEditorProps(category: "GameScripted/", description: "AO Limit component")]
class TILW_AOLimitComponentClass : ScriptComponentClass
{
}

class TILW_AOLimitComponent : ScriptComponent
{
	// Logic

	[Attribute("20", UIWidgets.Auto, "After how many seconds outside of AO players are killed", params: "0 inf 0", category: "Logic")]
	protected float m_killTimer;

	[RplProp(onRplName: "DrawAO"), Attribute("", UIWidgets.Auto, desc: "Factions affected by the AO limit (if empty, all factions)", category: "Logic")]
	protected ref array<string> m_factionKeys;

	[Attribute("", UIWidgets.Auto, desc: "Passengers of these vehicle prefabs (or inheriting) are NOT affected by the AO limit", category: "Logic")]
	protected ref array<ResourceName> m_ignoredVehicles;
	
	//[Attribute("", UIWidgets.Auto, desc: "Members of these groups (referenced by name) are not affected by the AO limit", category: "Logic")]
	protected ref array<string> m_ignoredGroups;

	protected float m_checkFrequency = 0.5;


	// Visualization

	[Attribute("0", UIWidgets.ComboBox, "Who can view the AO limit (everyone, affected factions, noone)", enums: ParamEnumArray.FromEnum(TILW_EVisibilityMode), category: "Visualization")]
	protected TILW_EVisibilityMode m_visibility;

	[Attribute("0 0 0 1", UIWidgets.ColorPicker, "The default color of the drawn AO limit line.", category: "Visualization")]
	protected ref Color m_defaultColor;

	[Attribute("1", UIWidgets.Auto, "If this AO limit only affects certain factions, use the faction color of the first one instead.", category: "Visualization")]
	protected bool m_useFactionColor;

	[Attribute("3", UIWidgets.Auto, "Width of the AO limit line.", params: "1 inf 0", category: "Visualization")]
	protected int m_lineWidth;



	[RplProp(onRplName: "OnPoints3DChange")]
	protected ref array<vector> m_points3D = new array<vector>();

	protected ref array<float> m_points2D = new array<float>();

	protected ref array<MapItem> m_markers = new array<MapItem>();

	protected float m_timeLeft = 0;
	protected float m_checkDelta = 0;

	protected bool m_wasOutsideAO = false;

	protected TILW_AOLimitDisplay m_aoLimitDisplay;

	void TILW_AOLimitComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		SetEventMask(ent, EntityEvent.INIT);
	}

	protected override void EOnInit(IEntity owner)
	{
		PolylineShapeEntity pse = PolylineShapeEntity.Cast(GetOwner());
		if (!pse) {
			Print("TILW_AOLimitComponent | Owner entity (" + GetOwner() + ") is not a polyline!", LogLevel.WARNING);
			return;
		}
		if (pse.GetPointCount() < 3) {
			Print("TILW_AOLimitComponent | Owner entity (" + GetOwner() + ") does not have enough points!", LogLevel.WARNING);
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

	protected override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		if (m_wasOutsideAO)
			UpdateTimer(timeSlice);

		m_checkDelta -= timeSlice;
		if (m_checkDelta > 0)
			return;
		m_checkDelta = m_checkFrequency;

		bool outsideAO = IsOutsideAO();
		if (!m_wasEverInsideAO && !outsideAO)
			m_wasEverInsideAO = true;
		
		if (m_wasOutsideAO == outsideAO || !m_wasEverInsideAO && outsideAO) // nothing changes, or nothing should happen
			return;
		
		if (outsideAO)
			PlayerLeavesAO(); // leaves ao
		else
			PlayerEntersAO(); // re-enters ao
		
		m_wasOutsideAO = outsideAO;
	}
	
	protected IEntity m_controlledEntity;
	protected bool m_wasEverInsideAO = false;

	protected bool IsOutsideAO()
	{
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gamemode.GetState() != SCR_EGameModeState.GAME)
			return false;

		if (!IsPlayerAffected())
			return false;

		IEntity ce = GetGame().GetPlayerController().GetControlledEntity();
		if (ce != m_controlledEntity)
		{
			PlayerEntersAO();
			m_wasOutsideAO = false;
			m_wasEverInsideAO = false;
			m_controlledEntity = ce;
		}
		vector playerPos = ce.GetOrigin();
		bool inPolygon = Math2D.IsPointInPolygon(m_points2D, playerPos[0], playerPos[2]);
		if (inPolygon)
			return false;

		IEntity ve = CompartmentAccessComponent.GetVehicleIn(ce);
		if (ve && IsVehicleIgnored(ve))
			return false;

		return true;
	}

	protected bool IsPlayerAffected()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return false;

		IEntity ce = GetGame().GetPlayerController().GetControlledEntity();
		if (!ce)
			return false;

		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(ce);
		if (!player)
			return false;

		if (!SCR_AIDamageHandling.IsAlive(player))
			return false;

		if (!player.m_pFactionComponent)
			return false;

		Faction faction = player.m_pFactionComponent.GetAffiliatedFaction();
		if (!faction)
			return false;

		if (!m_factionKeys.IsEmpty() && !m_factionKeys.Contains(faction.GetFactionKey()))
			return false;

		return true;
	}

	protected bool IsVehicleIgnored(IEntity e)
	{
		// Possible optimization: Save result for this vehicle

		if (m_ignoredVehicles.IsEmpty())
			return false;

		EntityPrefabData epd = e.GetPrefabData();
		if (!epd)
			return false;
		BaseContainer bc = epd.GetPrefab();
		if (!bc)
			return false;
		foreach (ResourceName rn : m_ignoredVehicles)
			if (SCR_BaseContainerTools.IsKindOf(bc, rn))
				return true;

		return false;
	}

	protected void UpdateTimer(float timeSlice)
	{
		m_timeLeft -= timeSlice;
		m_aoLimitDisplay.SetTime(m_timeLeft);

		if (m_timeLeft < 0)
		{
			IEntity player = GetGame().GetPlayerController().GetControlledEntity();
			CharacterControllerComponent characterController = CharacterControllerComponent.Cast(player.FindComponent(CharacterControllerComponent));
			if (characterController)
				characterController.ForceDeath();

			PlayerEntersAO();
		}
	}

	protected void PlayerLeavesAO()
	{
		m_timeLeft = m_killTimer;

		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.HINT);

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		SCR_HUDManagerComponent hud = SCR_HUDManagerComponent.Cast(playerController.GetHUDManagerComponent());
		m_aoLimitDisplay = TILW_AOLimitDisplay.Cast(hud.FindInfoDisplay(TILW_AOLimitDisplay));
		m_aoLimitDisplay.SetTime(m_timeLeft);
		m_aoLimitDisplay.Show(true, UIConstants.FADE_RATE_INSTANT);
	}

	protected void PlayerEntersAO()
	{
		m_timeLeft = m_killTimer;
		if (m_aoLimitDisplay)
			m_aoLimitDisplay.Show(false, UIConstants.FADE_RATE_INSTANT);
	}
	
	override void OnDelete(IEntity owner)
	{
		PlayerEntersAO();
		super.OnDelete(owner);
	}

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

		MapLink link = lastItem.LinkTo(firstItem);
		MapLinkProps linkProps = link.GetMapLinkProps();

		linkProps.SetLineColor(c);
		linkProps.SetLineWidth(m_lineWidth);
	}
}

enum TILW_EVisibilityMode
{
	ALL = 0,
	FACTION = 1,
	NONE = 2
}
