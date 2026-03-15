//! TILW_BaseTerm provides inverting functionality. Do not use this directly.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Base Term (NOT USEFUL)")]
class TILW_BaseTerm
{
	[Attribute("0", UIWidgets.Auto, desc: "Locally invert this term")]
	protected bool m_invertTerm;
	
	bool Eval()
	{
		if (m_invertTerm)
			return !Calc();
		else
			return Calc();
	}
	
	bool Calc()
	{
		return false;
	}
	
	void SetInvert(bool invert)
	{
		m_invertTerm = invert;
	}
}

//! TILW_OperationTerm adds an operand array to TILW_BaseTerm. Do not use this directly.
[BaseContainerProps(), BaseContainerCustomStringTitleField("Base Operation (NOT USEFUL)")]
class TILW_OperationTerm : TILW_BaseTerm
{
	[Attribute("", UIWidgets.Object, desc: "Operands used for the operation - either literals or sub-operations.")]
	ref array<ref TILW_BaseTerm> m_operands;
}


//! TILW_ConjunctionTerm returns true if all operands are true
[BaseContainerProps(), BaseContainerCustomStringTitleField("Conjunction (And/All)")]
class TILW_ConjunctionTerm : TILW_OperationTerm
{
	override bool Calc()
	{
		foreach (TILW_BaseTerm term: m_operands)
		{
			if (!term.Eval())
				return false;
		}
		return true;
	}
}

//! TILW_DisjunctionTerm returns true if any operands (at least one) are true
[BaseContainerProps(), BaseContainerCustomStringTitleField("Disjunction (Or/Any)")]
class TILW_DisjunctionTerm : TILW_OperationTerm
{
	override bool Calc()
	{
		foreach (TILW_BaseTerm term: m_operands)
		{
			if (term.Eval())
				return true;
		}
		return false;
	}
}

//! TILW_MinjunctionTerm returns true if at least m_minTrue operands are true
[BaseContainerProps(), BaseContainerCustomStringTitleField("Minjunction (At least k)")]
class TILW_MinjunctionTerm : TILW_OperationTerm
{
	[Attribute("1", UIWidgets.Auto, desc: "Minimum number of operands that must be true", params: "0 inf 1")]
	protected int m_minTrue;
	
	override bool Calc()
	{
		int numTrue = 0;
		foreach (TILW_BaseTerm term: m_operands)
		{
			if (term.Eval())
				numTrue += 1;
			if (numTrue >= m_minTrue)
				return true;
		}
		return false;
	}
}

//! TILW_EquivalenceTerm returns true if operands are either all true or all false
[BaseContainerProps(), BaseContainerCustomStringTitleField("Equivalence (all the same)")]
class TILW_EquivalenceTerm : TILW_OperationTerm
{
	override bool Calc()
	{	
		bool expected = m_operands[0].Eval();
		for (int i = 1; i < m_operands.Count(); i++)
			if (m_operands[i].Eval() != expected)
				return false;
		return true;
	}
}

//! TILW_MaxjunctionTerm returns true if no more than m_maxTrue operands are true
[BaseContainerProps(), BaseContainerCustomStringTitleField("Maxjunction (At most k)")]
class TILW_MaxjunctionTerm : TILW_OperationTerm
{
	[Attribute("1", UIWidgets.Auto, desc: "Maximum number of operands that may be true", params: "0 inf 1")]
	protected int m_maxTrue;
	
	override bool Calc()
	{
		int numTrue = 0;
		foreach (TILW_BaseTerm term: m_operands)
		{
			if (term.Eval())
				numTrue += 1;
			if (numTrue > m_maxTrue)
				return false;
		}
		return true;
	}
}

//! TILW_MatchjunctionTerm returns true if exactly m_matchTrue operands are true
[BaseContainerProps(), BaseContainerCustomStringTitleField("Matchjunction (Exactly k)")]
class TILW_MatchjunctionTerm : TILW_OperationTerm
{
	[Attribute("1", UIWidgets.Auto, desc: "Exact number of operands that have to be true", params: "0 inf 1")]
	protected int m_matchTrue;
	
	override bool Calc()
	{
		int numTrue = 0;
		foreach (TILW_BaseTerm term: m_operands)
		{
			if (term.Eval())
				numTrue += 1;
		}
		return (numTrue == m_matchTrue);
	}
}

//! TILW_LiteralTerm gets a value from the TILW_MissionFrameworkEntity, which was previously set by another entity, checks it matches specified value
[BaseContainerProps(), BaseContainerCustomStringTitleField("Literal (Mission Flag)")]
class TILW_LiteralTerm : TILW_BaseTerm
{
	[Attribute("", UIWidgets.Auto, desc: "Name of the literal (if not found, false)")]
	protected string m_flagName;
	
	override bool Calc()
	{
		return TILW_MissionFrameworkEntity.GetInstance().GetMissionFlag(m_flagName);
	}
	
	void SetFlag(string flagName)
	{
		m_flagName = flagName;
	}
}

//! TILW_GreaterTerm gets a value from the TILW_MissionFrameworkEntity, which was previously set by another entity, checks it is greater than specified value
[BaseContainerProps(), BaseContainerCustomStringTitleField("Greater Than")]
class TILW_GreaterTerm : TILW_BaseTerm
{
	[Attribute("", UIWidgets.Auto, desc: "Name of the flag (if not found, false)")]
	protected string m_flagName;
	[Attribute("1", UIWidgets.Auto, desc: "Exact number the flag has to be greater than", params: "0 inf 1")]
	protected int m_matchGreater;
	
	override bool Calc()
	{
		return TILW_MissionFrameworkEntity.GetInstance().GetMissionFlag(m_flagName) > m_matchGreater;
	}
}

//! TILW_LessTerm gets a value from the TILW_MissionFrameworkEntity, which was previously set by another entity, checks it is less than specified value
[BaseContainerProps(), BaseContainerCustomStringTitleField("Less Than")]
class TILW_LessTerm : TILW_BaseTerm
{
	[Attribute("", UIWidgets.Auto, desc: "Name of the flag (if not found, false)")]
	protected string m_flagName;
	[Attribute("1", UIWidgets.Auto, desc: "Exact number the flag has to be less than", params: "0 inf 1")]
	protected int m_matchLess;
	
	override bool Calc()
	{
		return TILW_MissionFrameworkEntity.GetInstance().GetMissionFlag(m_flagName) < m_matchLess;
	}
}
