[ComponentEditorProps(category: "GameScripted/", description: "AO Limit component")]
class TILW_AOLimitComponentClass : ScriptComponentClass
{
}

enum TILW_EIgnoreVehicles
{
	All = 1,
	Air
}

class TILW_AOLimitComponent : ScriptComponent
{
	[Attribute("30", UIWidgets.Auto, "How many seconds until the player is killed", params: "0 inf 0")]
	protected float maxTimeOutsideAO;
	
	[Attribute("", UIWidgets.Auto, desc: "Factions affected by the AO limit (if empty, all factions)")]
	protected ref array<string> m_factionKeys;
	
	[Attribute("", UIWidgets.Auto, desc: "These vehicle prefabs are NOT affected by the AO limit")]
	protected ref array<ResourceName> ignoreVehicles;
	
	[Attribute("", UIWidgets.ComboBox, desc: "Type of vehicle NOT affected by the AO limit", enums: ParamEnumArray.FromEnum(TILW_EIgnoreVehicles))]
	protected TILW_EIgnoreVehicles ignoreVehicleType;
	
	[Attribute("1", UIWidgets.Auto, "How many seconds pass between checking if players are still in AO", params: "0.25 inf 0.25")]
	protected int checkFrequency;
	
	protected ref array<vector> points3D = new array<vector>();
	protected ref array<float> points2D = new array<float>();
	protected float timer = 0;
	protected float nextUntil = 0;
	protected bool isOutsideAO = false;
	protected TILW_AOLimit aoLimitHud;
	
	void TILW_AOLimitComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		SetEventMask(ent, EntityEvent.INIT);
	}
	
	protected override void EOnInit(IEntity owner)
	{
		PolylineShapeEntity pse = PolylineShapeEntity.Cast(owner);
		if (!pse) {
			Print("TILW_AOLimitComponent | Owner entity (" + owner + ") is not a polyline!", LogLevel.WARNING);
			return;
		}
		if (pse.GetPointCount() < 3) {
			Print("TILW_AOLimitComponent | Owner entity (" + owner + ") does not have enough points!", LogLevel.WARNING);
			return;
		}

		pse.GetPointsPositions(points3D);
		for (int i = 0; i < points3D.Count(); i++) points3D[i] = pse.CoordToParent(points3D[i]);
		if(points2D.Count() == 0)
			SCR_Math2D.Get2DPolygon(points3D, points2D);
		
		if(RplSession.Mode() == RplMode.Dedicated)
			return;

		SetEventMask(owner, EntityEvent.FIXEDFRAME);
	}
	
	protected override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		nextUntil -= timeSlice;
		if(nextUntil > 0 && !isOutsideAO)
			return;
		nextUntil = checkFrequency;

		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gamemode.GetState() != SCR_EGameModeState.GAME)
			return;

		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(GetGame().GetPlayerController().GetControlledEntity());
		if(!player)
			return;

		if(!SCR_AIDamageHandling.IsAlive(player))
			return;
		
		if(!player.m_pFactionComponent)
			return;
		
		Faction faction = player.m_pFactionComponent.GetAffiliatedFaction();
		if(!faction)
			return;
		
		if(!m_factionKeys && !m_factionKeys.Contains(faction.GetFactionKey()))
			return;

		IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(player);
		if (vehicle)
		{
			VehicleHelicopterSimulation isAirVehicle;
			if(ignoreVehicleType == TILW_EIgnoreVehicles.Air)
				isAirVehicle = VehicleHelicopterSimulation.Cast(vehicle.FindComponent(VehicleHelicopterSimulation));

			if(ignoreVehicleType == TILW_EIgnoreVehicles.All || isAirVehicle || ignoreVehicles.Contains(vehicle.GetPrefabData().GetPrefabName()))
			{
				if(isOutsideAO)
					PlayerInsideAO();
				return;
			}
		}
		
		vector position = player.GetOrigin();
		bool inPolygon = Math2D.IsPointInPolygon(points2D, position[0], position[2]);
		
		if(inPolygon && isOutsideAO)
		{
			PlayerInsideAO();
			return;
		}
		
		if(!inPolygon && !isOutsideAO)
		{
			PlayerOutsideAO();
		}
		
		if(isOutsideAO)
			UpdateTimer(timeSlice);
	}
	
	protected void UpdateTimer(float timeSlice)
	{
		timer -= timeSlice;
	
		if(timer < 0)
		{
			IEntity player = GetGame().GetPlayerController().GetControlledEntity();
			CharacterControllerComponent characterController = CharacterControllerComponent.Cast(player.FindComponent(CharacterControllerComponent));
			if (characterController)
				characterController.ForceDeath();
			
			PlayerInsideAO();
		}
		aoLimitHud.SetTime(timer);
	}
	
	protected void PlayerOutsideAO()
	{
		isOutsideAO = true;
		timer = maxTimeOutsideAO;
		
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.HINT);
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		SCR_HUDManagerComponent hud = SCR_HUDManagerComponent.Cast(playerController.GetHUDManagerComponent());
		aoLimitHud = TILW_AOLimit.Cast(hud.FindInfoDisplay(TILW_AOLimit));
		aoLimitHud.SetTime(timer);
		aoLimitHud.Show(true,UIConstants.FADE_RATE_INSTANT);
	}
	
	protected void PlayerInsideAO()
	{
		isOutsideAO = false;
		aoLimitHud.Show(false,UIConstants.FADE_RATE_INSTANT);
	}
	
	void SetFactions(array<string> factions)
	{
		RpcDo_SetFactions(factions);
		Rpc(RpcDo_SetFactions, factions);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetFactions(array<string> factions)
	{
		m_factionKeys = factions;
	}
	
	void SetPoints(array<vector> points)
	{
		if (points.Count() < 3) {
			Print("TILW_AOLimitComponent | not enough points!", LogLevel.ERROR);
			return;
		}
		RpcDo_SetPoints(points);
		Rpc(RpcDo_SetPoints, points);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetPoints(array<vector> points)
	{
		m_points3D = points;
		SCR_Math2D.Get2DPolygon(points, m_points2D);
		
		if(RplSession.Mode() == RplMode.Dedicated)
			return;

		EntityEvent mask = GetEventMask();
		if(mask != EntityEvent.FIXEDFRAME)
			SetEventMask(GetOwner(), EntityEvent.FIXEDFRAME);
	}
	
	/*
	void DrawAO()
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		MapItem lastItem;
		MapItem firstItem;
		
		foreach(vector point : points3D)
		{
			MapItem item =  mapEntity.CreateCustomMapItem();
			item.SetBaseType(EMapDescriptorType.MDT_ICON);
			item.SetImageDef("editor-camera");
			item.SetPos(point[0],point[2]);
			item.SetVisible(true);
			markers.Insert(item);

			MapDescriptorProps props = item.GetProps();
			
			if(lastItem)
			{
				MapLink link = item.LinkTo(lastItem);
				MapLinkProps linkProps = link.GetMapLinkProps();
				linkProps.SetLineColor(AOColor);
				linkProps.SetLineWidth(AOLineWidth);
			}
			
			if(!firstItem)
				firstItem = item;
			lastItem = item;
		}
		
		MapLink link = lastItem.LinkTo(firstItem);
		MapLinkProps linkProps = link.GetMapLinkProps();
		
		linkProps.SetLineColor(AOColor);
		linkProps.SetLineWidth(AOLineWidth);
	}
	*/
}
