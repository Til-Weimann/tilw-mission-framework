[ComponentEditorProps(category: "GameScripted/Damage", description: "Attach to a PolylineShapeEntity. Players inside the polygon (and the vehicle they are in) take no damage. Optional faction filter. Server-authoritative.")]
class TILW_SafeZoneComponentClass : ScriptComponentClass {}

class TILW_SafeZoneComponent : ScriptComponent
{
	// How often (ms) player positions are checked.
	protected const int CHECK_INTERVAL_MS = 500;

	[Attribute("50.0", uiwidget: UIWidgets.Slider, params: "1 1000", desc: "Vertical range (m) above and below the polyline's height in which a player counts as inside the zone.")]
	float m_fVerticalRange;

	[Attribute("", uiwidget: UIWidgets.EditBox, desc: "Faction key (e.g. US, USSR, FIA). If set, ONLY players of this faction are protected. Leave empty to protect everyone. Case-sensitive.")]
	string m_sFactionKey;

	// All active safe zones (server only). Used so overlapping zones don't strip each other's protection.
	protected static ref array<TILW_SafeZoneComponent> s_aZones;

	// Polygon in world space (XZ), built from the polyline points.
	protected ref array<float> m_aPolyX = new array<float>();
	protected ref array<float> m_aPolyZ = new array<float>();
	protected float m_fReferenceY;
	protected bool m_bPolygonReady;
	protected bool m_bErrorLogged;

	// Everything (characters and vehicles) this zone currently has damage-disabled.
	protected ref array<IEntity> m_aProtected = new array<IEntity>();

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (SCR_Global.IsEditMode())
			return;

		// Server only. Clients never touch damage state.
		if (!Replication.IsServer())
			return;

		if (!s_aZones)
			s_aZones = new array<TILW_SafeZoneComponent>();
		s_aZones.Insert(this);

		GetGame().GetCallqueue().CallLater(CheckZone, CHECK_INTERVAL_MS, true);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		ScriptCallQueue queue = GetGame().GetCallqueue();
		if (queue)
			queue.Remove(CheckZone);

		// Give everything its damage back before this zone disappears
		// (unless another zone still wants it protected).
		array<IEntity> toRelease = {};
		foreach (IEntity tracked : m_aProtected)
			toRelease.Insert(tracked);

		m_aProtected.Clear();

		foreach (IEntity entity : toRelease)
		{
			if (entity)
				ReleaseEntity(entity);
		}

		if (s_aZones)
			s_aZones.RemoveItem(this);

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	// Builds the world-space polygon from the polyline shape. Returns false if not (yet) possible.
	protected bool BuildPolygon()
	{
		ShapeEntity shape = ShapeEntity.Cast(GetOwner());
		if (!shape)
		{
			if (!m_bErrorLogged)
			{
				Print("[TILW_SafeZone] Owner is not a ShapeEntity (use a PolylineShapeEntity). Zone inactive.", LogLevel.ERROR);
				m_bErrorLogged = true;
			}
			return false;
		}

		array<vector> points = {};
		shape.GetPointsPositions(points);

		if (points.Count() < 3)
		{
			if (!m_bErrorLogged)
			{
				Print("[TILW_SafeZone] Polyline needs at least 3 points. Zone inactive.", LogLevel.ERROR);
				m_bErrorLogged = true;
			}
			return false;
		}

		m_aPolyX.Clear();
		m_aPolyZ.Clear();

		float sumY = 0;
		foreach (vector localPoint : points)
		{
			vector worldPoint = GetOwner().CoordToParent(localPoint);
			m_aPolyX.Insert(worldPoint[0]);
			m_aPolyZ.Insert(worldPoint[2]);
			sumY += worldPoint[1];
		}

		m_fReferenceY = sumY / points.Count();
		m_bPolygonReady = true;
		m_bErrorLogged = false;

		Print("[TILW_SafeZone] Initialised on " + GetOwner().GetName() + " (" + points.Count() + " points).");
		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Point-in-polygon on the XZ plane (ray casting) plus a vertical range check.
	bool ContainsPoint(vector worldPos)
	{
		if (!m_bPolygonReady)
			return false;

		if (Math.AbsFloat(worldPos[1] - m_fReferenceY) > m_fVerticalRange)
			return false;

		float x = worldPos[0];
		float z = worldPos[2];
		int count = m_aPolyX.Count();

		bool inside = false;
		int j = count - 1;
		for (int i = 0; i < count; i++)
		{
			float xi = m_aPolyX[i];
			float zi = m_aPolyZ[i];
			float xj = m_aPolyX[j];
			float zj = m_aPolyZ[j];

			if (((zi > z) != (zj > z)) && (x < (xj - xi) * (z - zi) / (zj - zi) + xi))
				inside = !inside;

			j = i;
		}

		return inside;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsAlive(IEntity entity)
	{
		DamageManagerComponent damageManager = DamageManagerComponent.Cast(entity.FindComponent(DamageManagerComponent));
		if (!damageManager)
			return false;

		return damageManager.GetState() != EDamageState.DESTROYED;
	}

	//------------------------------------------------------------------------------------------------
	protected void SetDamageEnabled(IEntity entity, bool enabled)
	{
		DamageManagerComponent damageManager = DamageManagerComponent.Cast(entity.FindComponent(DamageManagerComponent));
		if (damageManager)
			damageManager.EnableDamageHandling(enabled);
	}

	//------------------------------------------------------------------------------------------------
	// True if the character's faction is allowed by this zone's faction filter.
	protected bool MatchesFaction(IEntity character)
	{
		if (m_sFactionKey.IsEmpty())
			return true;

		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(character.FindComponent(FactionAffiliationComponent));
		if (!affiliation)
			return false;

		string key = affiliation.GetAffiliatedFactionKey();
		return key == m_sFactionKey;
	}

	//------------------------------------------------------------------------------------------------
	// Works out everything this zone wants protected right now:
	// every living, faction-matching player inside the zone, plus the vehicle they are in.
	// A player in a vehicle counts as inside when the VEHICLE is inside the zone.
	protected void CollectDesired(notnull array<IEntity> desired)
	{
		if (!m_bPolygonReady)
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);

		foreach (int playerId : playerIds)
		{
			IEntity character = playerManager.GetPlayerControlledEntity(playerId);
			if (!character)
				continue;

			if (!IsAlive(character) || !MatchesFaction(character))
				continue;

			IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(character);

			IEntity anchor = character;
			if (vehicle)
				anchor = vehicle;

			if (!ContainsPoint(anchor.GetOrigin()))
				continue;

			if (desired.Find(character) == -1)
				desired.Insert(character);

			if (vehicle && desired.Find(vehicle) == -1)
				desired.Insert(vehicle);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Does this zone, right now, want this entity protected? Used by other zones on handover.
	bool WantsToProtect(IEntity entity)
	{
		if (!entity)
			return false;

		array<IEntity> desired = {};
		CollectDesired(desired);

		return desired.Find(entity) != -1;
	}

	//------------------------------------------------------------------------------------------------
	// Another zone is giving up an entity that this zone still wants: take it over without a gap.
	void AdoptProtection(IEntity entity)
	{
		if (!entity)
			return;

		SetDamageEnabled(entity, false);

		if (m_aProtected.Find(entity) == -1)
			m_aProtected.Insert(entity);
	}

	//------------------------------------------------------------------------------------------------
	protected void ProtectEntity(IEntity entity)
	{
		SetDamageEnabled(entity, false);
		m_aProtected.Insert(entity);
	}

	//------------------------------------------------------------------------------------------------
	// Called after the entity has been removed from this zone's list.
	protected void ReleaseEntity(IEntity entity)
	{
		// If another zone still wants this entity protected, hand it over instead of restoring damage.
		if (s_aZones)
		{
			foreach (TILW_SafeZoneComponent zone : s_aZones)
			{
				if (zone && zone != this && zone.WantsToProtect(entity))
				{
					zone.AdoptProtection(entity);
					return;
				}
			}
		}

		SetDamageEnabled(entity, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckZone()
	{
		if (!Replication.IsServer())
			return;

		if (!m_bPolygonReady && !BuildPolygon())
			return;

		array<IEntity> desired = {};
		CollectDesired(desired);

		// Release anything we protect that is no longer wanted (also drops deleted entities).
		for (int i = m_aProtected.Count() - 1; i >= 0; i--)
		{
			IEntity current = m_aProtected[i];

			if (!current)
			{
				m_aProtected.RemoveOrdered(i);
				continue;
			}

			if (desired.Find(current) == -1)
			{
				m_aProtected.RemoveOrdered(i);
				ReleaseEntity(current);
			}
		}

		// Protect anything newly wanted.
		foreach (IEntity wanted : desired)
		{
			if (m_aProtected.Find(wanted) == -1)
				ProtectEntity(wanted);
		}
	}
}
