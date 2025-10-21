enum TILW_EAoEffect
{
	NEUTRAL,    // Default - normal AO rules
    EXEMPT,     // IGNORE AO completely
    AFFECTED    // FORCE AO always
}

[ComponentEditorProps(category: "GameScripted/", description: "AO Limit component")]
class TILW_AOLimitComponentClass : ScriptComponentClass
{
}

class TILW_AOLimitComponent : ScriptComponent
{
	// Logic
	[Attribute("", UIWidgets.Auto, desc: "Inverts area AO check to consider players outside the AO shap outside.", category: "Logic")]
	protected bool m_invertArea;
	
	[Attribute("", UIWidgets.Auto, desc: "This will apply the AO to users never in the AO", category: "Logic")]
	protected bool m_effectUsersNeverInsideAO;
	
	[Attribute("", UIWidgets.Auto, desc: "Factions affected by the AO limit (if empty, all factions)", category: "Logic")]
	protected ref array<string> m_factionKeys;

	//[Attribute("", UIWidgets.Auto, desc: "Passengers of these vehicle prefabs (or inheriting) are NOT affected by the AO limit", category: "Logic")]
	//protected ref array<ResourceName> m_ignoredVehicles;
	
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Prefabs that EXEMPT users from this AO limit affects (in inventory, vehicle inventory, or passenger)", category: "Logic")]
	protected ref array<ResourceName> m_exemptPrefabs;
	
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Prefabs that EFFECT users in this AO limit (in inventory, vehicle inventory, or passenger)", category: "Logic")]
	protected ref array<ResourceName> m_affectedPrefabs;
	
	[Attribute("", UIWidgets.Auto, desc: "Names of entities that EXEMPT users from this AO limit affects (in inventory, vehicle inventory, or passenger)", category: "Logic")]
	protected ref array<string> m_exemptEntityNames;
	
	[Attribute("", UIWidgets.Auto, desc: "Names of entities that EFFECT users in this AO limit (in inventory, vehicle inventory, or passenger)", category: "Logic")]
	protected ref array<string> m_affectedEntityNames;
	
	//[Attribute("", UIWidgets.Auto, desc: "Members of these groups (referenced by name) are not affected by the AO limit", category: "Logic")]
	protected ref array<string> m_ignoredGroups;

	// Visualization
	[Attribute("0", UIWidgets.ComboBox, "Who can view the AO limit (everyone, affected factions, noone)", enums: ParamEnumArray.FromEnum(TILW_EVisibilityMode), category: "Visualization")]
	protected TILW_EVisibilityMode m_visibility;

	[Attribute("0 0 0 1", UIWidgets.ColorPicker, "The default color of the drawn AO limit line.", category: "Visualization")]
	protected ref Color m_defaultColor;

	[Attribute("1", UIWidgets.Auto, "If this AO limit only affects certain factions, use the faction color of the first one instead.", category: "Visualization")]
	protected bool m_useFactionColor;

	[Attribute("3", UIWidgets.Auto, "Width of the AO limit line.", params: "1 inf 0", category: "Visualization")]
	protected int m_lineWidth;

	protected ref array<vector> m_points3D = new array<vector>();

	protected ref array<MapItem> m_markers = new array<MapItem>();
	
	protected ref map<IEntity,TILW_EAoEffect> m_entitiesCache = new map<IEntity,TILW_EAoEffect>();
	
	protected bool m_wasEverInsideAO = false;
	
	void TILW_AOLimitComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		SetEventMask(ent, EntityEvent.INIT);
	}

	protected override void EOnInit(IEntity owner)
	{
		PolylineShapeEntity pse = PolylineShapeEntity.Cast(GetOwner());
		if (!pse)
		{
			Print("TILW_AOLimitComponent | Owner entity (" + GetOwner() + ") is not a polyline!", LogLevel.ERROR);
			return;
		}
		if (pse.GetPointCount() < 3)
		{
			Print("TILW_AOLimitComponent | Owner entity (" + GetOwner() + ") does not have enough points!", LogLevel.ERROR);
			return;
		}
		
		if(!GetGame().InPlayMode())
			return;

		pse.GetPointsPositions(m_points3D);
		for (int i = 0; i < m_points3D.Count(); i++)
			m_points3D[i] = pse.CoordToParent(m_points3D[i]);
	
		if (RplSession.Mode() == RplMode.Dedicated)
			return;
		
		TILW_AOLimitManager.GetInstance().Register(this);
		DrawAO();

		if (m_visibility == TILW_EVisibilityMode.FACTION)
		{
			PS_PlayableManager pm = PS_PlayableManager.GetInstance();
			pm.GetOnFactionChange().Insert(OnFactionChange);
		}
	}
	
	bool IsPlayerSafe()
	{
		PlayerController pc = GetGame().GetPlayerController();
		
		if (!IsPlayerAffected(pc))
	        return true;
		
		bool inArea = IsPlayerPosInsideArea(pc);
	    if (!m_wasEverInsideAO && inArea)
		    m_wasEverInsideAO = true;
		
		if(!m_wasEverInsideAO && !m_effectUsersNeverInsideAO)
			return true;

		TILW_EAoEffect effect = GetPlayerEffect(pc);
		switch (effect)
	    {
	        case TILW_EAoEffect.EXEMPT:   return true;
	        case TILW_EAoEffect.AFFECTED: return false;
	        case TILW_EAoEffect.NEUTRAL:  return inArea;
	        default:                      return false;
	    }
		
	    return false;
	}
	
	protected bool IsPlayerPosInsideArea(notnull PlayerController pc)
	{
		IEntity player = pc.GetControlledEntity();
		if (!player)
			return false;
	
		vector playerPos = player.GetOrigin();
		bool inPolygon = Math2D.IsPointInPolygonXZ(m_points3D, playerPos);
	
		if (!m_wasEverInsideAO)
		{
			if ((inPolygon && !m_invertArea) || (!inPolygon && m_invertArea))
				m_wasEverInsideAO = true;
		}
	
		bool inside = inPolygon;
		if (m_invertArea)
			inside = !inside;

		return inside;
	}

	protected bool IsPlayerAffected(notnull PlayerController pc)
	{
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
	    if (gamemode.GetState() != SCR_EGameModeState.GAME)
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
	
	protected TILW_EAoEffect GetEntityEffect(notnull IEntity entity)
	{
		TILW_EAoEffect cached;
	    if (m_entitiesCache.Find(entity, cached))
	        return cached;
	
	    // PRIORITY 1: EXEMPT (wins always)
	    if (!m_exemptPrefabs.IsEmpty() && ArrayContainsKind(entity, m_exemptPrefabs))
	    {
	        m_entitiesCache.Insert(entity, TILW_EAoEffect.EXEMPT);
	        return TILW_EAoEffect.EXEMPT;
	    }
	    if (!m_exemptEntityNames.IsEmpty() && m_exemptEntityNames.Contains(entity.GetName()))
	    {
	        m_entitiesCache.Insert(entity, TILW_EAoEffect.EXEMPT);
	        return TILW_EAoEffect.EXEMPT;
	    }
	
	    // PRIORITY 2: AFFECTED
	    if (!m_affectedPrefabs.IsEmpty() && ArrayContainsKind(entity, m_affectedPrefabs))
	    {
	        m_entitiesCache.Insert(entity, TILW_EAoEffect.AFFECTED);
	        return TILW_EAoEffect.AFFECTED;
	    }
	    if (!m_affectedEntityNames.IsEmpty() && m_affectedEntityNames.Contains(entity.GetName()))
	    {
	        m_entitiesCache.Insert(entity, TILW_EAoEffect.AFFECTED);
	        return TILW_EAoEffect.AFFECTED;
	    }
	
	    // DEFAULT: NEUTRAL
	    m_entitiesCache.Insert(entity, TILW_EAoEffect.NEUTRAL);
	    return TILW_EAoEffect.NEUTRAL;
	}
	
	protected TILW_EAoEffect GetPlayerEffect(notnull PlayerController pc)
    {
        bool hasExempt = false;
        bool hasAffected = false;

		IEntity player = pc.GetControlledEntity();
		if(!player)
			return TILW_EAoEffect.NEUTRAL;
		
        // Check player's inventory
        SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
        if (inv)
        {
			array<IEntity> items = {};
			inv.GetAllRootItems(items);
			
			foreach (IEntity item : items)
			{
				TILW_EAoEffect effect = GetEntityEffect(item);
				if (effect == TILW_EAoEffect.EXEMPT)
				    hasExempt = true;
				else if (effect == TILW_EAoEffect.AFFECTED)
				    hasAffected = true;
			}
        }

        // Check if in vehicle
        IEntity ve = CompartmentAccessComponent.GetVehicleIn(player);
        if (ve)
        {
            TILW_EAoEffect vehEffect = GetVehicleEffect(ve);
            if (vehEffect == TILW_EAoEffect.EXEMPT)
                hasExempt = true;
            else if (vehEffect == TILW_EAoEffect.AFFECTED)
                hasAffected = true;
        }

        if (hasExempt)
            return TILW_EAoEffect.EXEMPT;
        if (hasAffected)
            return TILW_EAoEffect.AFFECTED;
        return TILW_EAoEffect.NEUTRAL;
    }
	
	protected TILW_EAoEffect GetVehicleEffect(notnull IEntity vehicle)
    {
        bool hasExempt = false;
        bool hasAffected = false;

        TILW_EAoEffect vehEffect = GetEntityEffect(vehicle);
        if (vehEffect == TILW_EAoEffect.EXEMPT)
            hasExempt = true;
        else if (vehEffect == TILW_EAoEffect.AFFECTED)
            hasAffected = true;

		// Check Inventory
        SCR_UniversalInventoryStorageComponent uisc = SCR_UniversalInventoryStorageComponent.Cast(vehicle.FindComponent(SCR_UniversalInventoryStorageComponent));
        if (uisc)
        {
			array<IEntity> items = {};
			uisc.GetAll(items);
			
			foreach (IEntity item : items)
			{
				TILW_EAoEffect effect = GetEntityEffect(item);
				if (effect == TILW_EAoEffect.EXEMPT)
					hasExempt = true;
				else if (effect == TILW_EAoEffect.AFFECTED)
					hasAffected = true;
			}
        }

        if (hasExempt)
            return TILW_EAoEffect.EXEMPT;
        if (hasAffected)
            return TILW_EAoEffect.AFFECTED;
        return TILW_EAoEffect.NEUTRAL;
    }
	
	protected void UnDrawAO()
	{
		foreach (MapItem marker : m_markers)
		{
			marker.SetVisible(false);
			marker.Recycle();
		}
		m_markers.Clear();
	}
	
	protected void DrawAO()
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		UnDrawAO();

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
	
	protected bool ArrayContainsKind(IEntity entity, array<ResourceName> list)
	{
		EntityPrefabData epd = entity.GetPrefabData();
		if (!epd)
			return false;

		BaseContainer bc = epd.GetPrefab();
		if (!bc)
			return false;
		
		foreach (ResourceName rn : list)
			if (SCR_BaseContainerTools.IsKindOf(bc, rn))
				return true;
		
		return false;
	}
	
	void SetFactions(array<string> factions)
	{
		if (SCR_ArrayHelperT<string>.AreEqual(factions, m_factionKeys))
			return;
		
		Rpc(RpcDo_SetFactions, factions);
		RpcDo_SetFactions(factions)
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetFactions(array<string> factions)
	{
		m_factionKeys = factions;

		if (RplSession.Mode() == RplMode.Dedicated)
			return;
		
		DrawAO();
	}

	void SetPoints(array<vector> points)
	{
		if (points.Count() < 3) {
			Print("TILW_AOLimitComponent | not enough points!", LogLevel.ERROR);
			return;
		}

		Rpc(RpcDo_SetPoints, points);
		RpcDo_SetPoints(points);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetPoints(array<vector> points)
	{
		if (SCR_ArrayHelperT<vector>.AreEqual(points, m_points3D))
			return;

		m_points3D = points;

		if (RplSession.Mode() == RplMode.Dedicated)
			return;
		
		EntityEvent mask = GetEventMask();
		if (mask != EntityEvent.FIXEDFRAME)
			SetEventMask(GetOwner(), EntityEvent.FIXEDFRAME);

		DrawAO();
	}
	
	void OnEntityChanged()
	{
		m_wasEverInsideAO = false;
	}
	
	void OnFactionChange(int playerId, FactionKey factionKey, FactionKey factionKeyOld)
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
	
	override void OnDelete(IEntity owner)
	{
		UnDrawAO();
		TILW_AOLimitManager.GetInstance().UnRegister(this);
		
		super.OnDelete(owner);
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
		
		return true;
	}
	
	override bool RplLoad(ScriptBitReader reader)
	{
		array<string> factionKeys = {};
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
		
		if(!SCR_ArrayHelperT<vector>.AreEqual(points, m_points3D) ||
		   !SCR_ArrayHelperT<string>.AreEqual(factionKeys, m_factionKeys))
		{
			m_factionKeys = factionKeys;
			m_points3D = points;
			DrawAO();
			SetEventMask(GetOwner(), EntityEvent.FIXEDFRAME);
		}
		
		return true;
	}
}