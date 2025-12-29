modded class SCR_TimeAndWeatherHandlerComponent : SCR_BaseGameModeComponent
{
	[Attribute("1", desc: "If it's a RL gamemode, reset time when players go ingame so briefing duration does not affect starting time.",category: "Time")]
	protected bool m_bResetTimeOnGameStart;
	
	[Attribute("0", category: "Override Date")]
	protected bool m_bUseSpecifiedDate;
	[Attribute("1", UIWidgets.Slider, "Specified day of month", "1 31 1", category: "Override Date")]
	protected int m_iSetDay;
	[Attribute("8", UIWidgets.Slider, "Specified month of year", "1 12 1", category: "Override Date")]
	protected int m_iSetMonth;
	[Attribute("1989", UIWidgets.Slider, "Specified year", "1901 2199 1", category: "Override Date")]
	protected int m_iSetYear;
	
	[Attribute("0", category: "Override Geocoords")]
	protected bool m_bOverrideGeocoords;
	[Attribute("49.019001", uiwidget:UIWidgets.Slider, params: "-90 90 1", category: "Override Geocoords")]
	protected float m_fLatitude;
	[Attribute("37.921001", uiwidget:UIWidgets.Slider, params: "-180 180 1", category: "Override Geocoords")]
	protected float m_fLongitude;

	[Attribute("0", category: "Override Time Zone")]
	protected bool m_bOverrideTimeZoneInfo;
	[Attribute("2", uiwidget:UIWidgets.Slider, params: "-12 12 0.25", category: "Override Time Zone")]
	protected float m_fUTCTimeZone;
	[Attribute(defvalue: "false", uiwidget:UIWidgets.CheckBox, category: "Override Time Zone")]
	protected bool m_fDSTEnabled;
	[Attribute("1", uiwidget:UIWidgets.Slider, params: "0 5 0.25", category: "Override Time Zone")]
	protected float m_fDSTOffsetHours;

	[Attribute("0", category: "Override Environment")]
	protected bool m_bOverrideEnvironment;
	[Attribute("1", uiwidget:UIWidgets.Slider, params: "0 1 0.01", category: "Override Environment")]
	protected float m_fFogDensity;
	[Attribute("1", uiwidget:UIWidgets.Slider, params: "0 150 0.1", category: "Override Environment")]
	protected float m_fFogHeight;
	[Attribute("1", uiwidget:UIWidgets.Slider, params: "0 1 0.01", category: "Override Environment")]
	protected float m_fRain;
	[Attribute("1", uiwidget:UIWidgets.Slider, params: "0 50 1", category: "Override Environment", desc:"Wind speed in m/s")]
	protected float m_fWind;
	
	//------------------------------------------------------------------------------------------------
	override void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);

		if (!Replication.IsServer() || !GetGame().InPlayMode())
			return;

		if (s_Instance != this)
		{
			Print("Multiple instances of SCR_TimeAndWeatherHandlerComponent detected.", LogLevel.WARNING);
			return;
		}
		
		ChimeraWorld chw = ChimeraWorld.CastFrom(GetOwner().GetWorld());
		if (!chw)
			return;
		
		TimeAndWeatherManagerEntity manager = chw.GetTimeAndWeatherManager();

		if (m_bUseSpecifiedDate)
		{
			manager.SetDate(m_iSetYear, m_iSetMonth, m_iSetDay, true);
		}
		if (m_bOverrideGeocoords)
		{
			manager.SetCurrentLatitude(m_fLatitude);
			manager.SetCurrentLongitude(m_fLongitude);
		}
		if (m_bOverrideTimeZoneInfo)
		{
			manager.SetTimeZoneOffset(m_fUTCTimeZone);
			manager.SetDSTOffset(m_fDSTOffsetHours);
			manager.SetDSTEnabled(m_fDSTEnabled)
		}
		if (m_bOverrideEnvironment)
		{
			manager.SetFogAmountOverride(true, m_fFogDensity);
			manager.SetFogHeightDensityOverride(true, m_fFogHeight);
			manager.SetRainIntensityOverride(true, m_fRain);
			manager.SetWindSpeedOverride(true, m_fWind);
		}
	}
	
	// save time
	
	protected int m_iSavedHours;
	protected int m_iSavedMinutes;
	protected int m_iSavedSeconds;
	
	override void SetupDaytimeAndWeather(int hours, int minutes, int seconds = 0, string loadedWeatherState = "", bool loadDone = false)
	{
		super.SetupDaytimeAndWeather(hours, minutes, seconds, loadedWeatherState, loadDone);
		
		if (!m_bResetTimeOnGameStart)
			return;
		
		ChimeraWorld world = ChimeraWorld.CastFrom(GetOwner().GetWorld());
		if (!world)
			return;
		
		TimeAndWeatherManagerEntity manager = world.GetTimeAndWeatherManager();
		if (!manager)
			return;
		
		PS_GameModeCoop gamemode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (!gamemode)
			return;
		
		manager.GetHoursMinutesSeconds(m_iSavedHours, m_iSavedMinutes, m_iSavedSeconds);
		
		gamemode.GetOnGameStateChange().Insert(TILW_GameStateChange);
	}
	
	protected void TILW_GameStateChange(SCR_EGameModeState state)
	{
		if (state != SCR_EGameModeState.GAME)
			return;
		
		ChimeraWorld world = ChimeraWorld.CastFrom(GetOwner().GetWorld());
		TimeAndWeatherManagerEntity manager = world.GetTimeAndWeatherManager();
		if (manager)
			manager.SetHoursMinutesSeconds(m_iSavedHours, m_iSavedMinutes, m_iSavedSeconds);
		
		PS_GameModeCoop gamemode = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		if (gamemode)
			gamemode.GetOnGameStateChange().Remove(TILW_GameStateChange);
	}
}