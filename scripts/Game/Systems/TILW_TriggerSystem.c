class TILW_TriggerSystem : GameSystem
{
	
	// Attributes ------------------------------------------------
	
	[Attribute(defvalue: "1000", desc: "Idle time between counts, in milliseconds")]
    private int m_updateFrequency;
	
	[Attribute(defvalue: "50", desc: "How many characters are allowed to be processed per tick")]
    private int m_batchSize;
	
	//! Provides instance of the trigger system
	static TILW_TriggerSystem GetInstance()
	{
		World world = GetGame().GetWorld();
		if (!world)
			return null;
		return TILW_TriggerSystem.Cast(world.FindSystem(TILW_TriggerSystem));
	}
	
	//! Initializes system
	override void OnInit()
	{
		super.OnInit();
		if (m_triggers.IsEmpty())
			Enable(false);
	}
	
	//! Initializes system information
	override static void InitInfo(WorldSystemInfo outInfo)
	{
		super.InitInfo(outInfo);
		outInfo
			.SetAbstract(false)
			.SetUnique(true)
			.SetLocation(ESystemLocation.Server)
			.AddPoint(ESystemPoint.Frame);
	}
	
	// Triggers and characters ------------------------------------------------
	
	//! Holds all triggers registered with the system
	protected ref array<TILW_BaseTriggerEntity> m_triggers = {};
	
	//! Holds all characters registered with the system
	protected ref array<SCR_ChimeraCharacter> m_characters = {};
	
	//! Holds triggers that should be registered with the system
	protected ref array<TILW_BaseTriggerEntity> m_triggersToInsert = {};
	
	//! Inserts a trigger into the system
	void InsertTrigger(TILW_BaseTriggerEntity trigger)
	{
		if (!trigger || m_triggers.Contains(trigger) || m_triggersToInsert.Contains(trigger))
			return;
		
		if (m_currentIndex == 0)
			m_triggers.Insert(trigger);
		else
			m_triggersToInsert.Insert(trigger);
		
		if (!IsEnabled())
			Enable(true);
	}
	
	//! Removes a trigger from the system
	void RemoveTrigger(TILW_BaseTriggerEntity trigger)
	{
		if (!m_triggers.RemoveItem(trigger))
			m_triggersToInsert.RemoveItem(trigger);
		
		if (m_triggers.IsEmpty())
			Enable(false);
	}
	
	//! Inserts a character into the system
	void InsertCharacter(SCR_ChimeraCharacter character)
	{
		if (!character || m_characters.Contains(character))
			return;
		m_characters.Insert(character);
	}
	
	//! Removes a character from the system
	void RemoveCharacter(SCR_ChimeraCharacter character)
	{
		int index = m_characters.Find(character);
		if (index == -1)
			return;
		
		m_characters.Remove(index);
		
		if (m_currentIndex > 0 && index < m_currentIndex && m_characters.Count() > index && m_characters[index])
			ProcessCharacter(m_characters[index]); // Process swapback character
	}
	
	// OnUpdate ------------------------------------------------
	
	//! Keeps track of when we last counted
	protected int m_lastUpdate = 0;
	
	//! Keeps track of how far we have counted
	protected int m_currentIndex = 0;
	
	//! Initiates character processing
	override protected void OnUpdate(ESystemPoint point)
	{
		super.OnUpdate(point);
		if (m_currentIndex > 0 || System.GetTickCount() - m_lastUpdate >= m_updateFrequency)
			ProcessCharacters();
	}
	
	//! Processes batches of characters
	protected void ProcessCharacters()
	{
		int count = m_characters.Count();
		int processedThisFrame = 0;
		
		while (m_currentIndex < count)
		{
			if (processedThisFrame >= m_batchSize)
				return;
			
			ProcessCharacter(m_characters[m_currentIndex]);
			
			m_currentIndex += 1;
			processedThisFrame += 1;
		}
		
		PostCharacterProcessing();
	}
	
	//! Hands a character to all triggers for counting
	protected void ProcessCharacter(SCR_ChimeraCharacter cc)
	{
		foreach (TILW_BaseTriggerEntity t : m_triggers)
			t.ProcessCharacter(cc);
	}
	
	//! Instructs all triggers to evaluate, and prepares system for next round
	protected void PostCharacterProcessing()
	{
		foreach (TILW_BaseTriggerEntity t : m_triggers)
			t.Eval();
		m_lastUpdate = System.GetTickCount();
		m_currentIndex = 0;
		
		// Register any new triggers
		foreach (TILW_BaseTriggerEntity t: m_triggersToInsert)
			m_triggers.Insert(t);
		m_triggersToInsert.Clear();
	}
	
	// Unfinished:
	// Polyline WB visualiuation
	
}