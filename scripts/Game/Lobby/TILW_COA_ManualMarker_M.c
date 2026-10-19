#ifdef COALITION_LOBBY
	// Coalition Lobby map markers - the framework's per-marker bring-to-front option, as for Reforger Lobby's
	modded class COA_ManualMarker
	{
		[Attribute("true")]
		bool m_bringToFront;
	}

	modded class COA_ManualMarkerComponent
	{
		override bool OnMouseEnter(Widget w, int x, int y)
		{
			if (!m_ManualMarker || m_ManualMarker.m_bringToFront)
				return super.OnMouseEnter(w, x, y);

			// Coalition Lobby's hover, without raising the marker above the others
			if (m_bJustText)
				return true;
			m_iZOrder = m_wRoot.GetZOrder();
			if (m_bHasGlow)
				m_wMarkerIconGlow.SetVisible(true);
			if (m_sDescription != "")
				m_wDescriptionPanel.SetVisible(true);

			return true;
		}
	}
#endif
