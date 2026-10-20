#ifdef PS_LOBBY
	// Reforger Lobby map markers
	modded class PS_ManualMarker
	{
		[Attribute("true")]
		protected bool m_bringToFront;

		override void CreateMapWidget(MapConfiguration mapConfig)
		{
			// If marker already exists ignore
			if (m_wRoot)
				return;

			// Faction visibility check
			if (!IsCurrentFactionVisibility())
				return;

			// Get map frame
			Widget mapFrame = m_MapEntity.GetMapMenuRoot().FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
			if (!mapFrame)
				mapFrame = m_MapEntity.GetMapMenuRoot();
			if (!mapFrame)
				return; // Somethig gone wrong

			// Create and init marker
			m_wRoot = GetGame().GetWorkspace().CreateWidgets(m_sMarkerPrefab, mapFrame);
			m_wRoot.SetZOrder(m_iZOrder);
			m_hManualMarkerComponent = PS_ManualMarkerComponent.Cast(m_wRoot.FindHandler(PS_ManualMarkerComponent));
			m_hManualMarkerComponent.SetImage(m_sImageSet, m_sQuadName);
			m_hManualMarkerComponent.SetImageGlow(m_sImageSetGlow, m_sQuadName);
			m_hManualMarkerComponent.SetDescription(m_sDescription);
			m_hManualMarkerComponent.SetColor(m_MarkerColor);
			m_hManualMarkerComponent.OnMouseLeave(null, null, 0, 0);
			m_hManualMarkerComponent.m_bringToFront = m_bringToFront;
			// Enable every frame updating
			SetEventMask(EntityEvent.POSTFRAME);
		}
	}
#endif
