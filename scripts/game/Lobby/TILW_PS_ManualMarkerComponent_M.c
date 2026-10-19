#ifdef PS_LOBBY
	// Reforger Lobby map markers
	modded class PS_ManualMarkerComponent
	{
		bool m_bringToFront = true;

		override bool OnMouseEnter(Widget w, int x, int y)
		{
			m_iZOrder = m_wRoot.GetZOrder();
			if (m_bringToFront)
				m_wRoot.SetZOrder(10000);
			if (m_bHasGlow)
				m_wMarkerIconGlow.SetVisible(true);
			if (m_sDescription != "")
				m_wDescriptionPanel.SetVisible(true);

			return true;
		}
	}
#endif
