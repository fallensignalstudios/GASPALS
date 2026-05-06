#include "Classes/SFDisciplineDefinition.h"

#include "Misc/DataValidation.h"

TArray<FSFDisciplineSkillNode> USFDisciplineDefinition::GetNodesForTier(ESFDisciplineTier Tier) const
{
	TArray<FSFDisciplineSkillNode> Result;
	for (const FSFDisciplineSkillNode& Node : SkillNodes)
	{
		if (Node.Tier == Tier)
		{
			Result.Add(Node);
		}
	}
	return Result;
}

bool USFDisciplineDefinition::AreTier1NodesComplete(const TArray<FName>& PurchasedNodeIDs) const
{
	for (const FSFDisciplineSkillNode& Node : SkillNodes)
	{
		if (Node.Tier == ESFDisciplineTier::Tier1 && !PurchasedNodeIDs.Contains(Node.NodeID))
		{
			return false;
		}
	}
	return true;
}

bool USFDisciplineDefinition::AreTier2NodesComplete(const TArray<FName>& PurchasedNodeIDs) const
{
	for (const FSFDisciplineSkillNode& Node : SkillNodes)
	{
		if (Node.Tier == ESFDisciplineTier::Tier2 && !PurchasedNodeIDs.Contains(Node.NodeID))
		{
			return false;
		}
	}
	return true;
}

int32 USFDisciplineDefinition::GetTotalPointCost() const
{
	int32 Total = 0;
	for (const FSFDisciplineSkillNode& Node : SkillNodes)
	{
		Total += Node.DisciplinePointCost;
	}
	return Total;
}

#if WITH_EDITOR

EDataValidationResult USFDisciplineDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	// Identity
	if (DisciplineName.IsNone())
	{
		Context.AddError(FText::FromString(
			FString::Printf(TEXT("%s: DisciplineName is not set."), *GetName())));
		Result = EDataValidationResult::Invalid;
	}

	if (DisciplineDisplayName.IsEmpty())
	{
		Context.AddError(FText::FromString(
			FString::Printf(TEXT("%s: DisciplineDisplayName is not set."), *GetName())));
		Result = EDataValidationResult::Invalid;
	}

	if (!DisciplineIcon)
	{
		Context.AddWarning(FText::FromString(
			FString::Printf(TEXT("%s: DisciplineIcon is not set."), *GetName())));
	}

	// Skill nodes
	if (SkillNodes.IsEmpty())
	{
		Context.AddError(FText::FromString(
			FString::Printf(TEXT("%s: No SkillNodes defined."), *GetName())));
		Result = EDataValidationResult::Invalid;
	}

	// Validate tier distribution
	int32 Tier1Count = 0;
	int32 Tier2Count = 0;
	int32 Tier3Count = 0;

	TSet<FName> NodeIDs;

	for (const FSFDisciplineSkillNode& Node : SkillNodes)
	{
		// Check for null ability
		if (!Node.GrantedAbility)
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("%s: SkillNode '%s' has no GrantedAbility."),
					*GetName(), *Node.NodeName.ToString())));
			Result = EDataValidationResult::Invalid;
		}

		// Check for duplicate node IDs
		if (Node.NodeID.IsNone())
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("%s: SkillNode '%s' has no NodeID set."),
					*GetName(), *Node.NodeName.ToString())));
			Result = EDataValidationResult::Invalid;
		}
		else if (NodeIDs.Contains(Node.NodeID))
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("%s: Duplicate NodeID '%s' found."),
					*GetName(), *Node.NodeID.ToString())));
			Result = EDataValidationResult::Invalid;
		}
		else
		{
			NodeIDs.Add(Node.NodeID);
		}

		// Count per tier
		switch (Node.Tier)
		{
		case ESFDisciplineTier::Tier1: Tier1Count++; break;
		case ESFDisciplineTier::Tier2: Tier2Count++; break;
		case ESFDisciplineTier::Tier3: Tier3Count++; break;
		}

		// Check cost
		if (Node.DisciplinePointCost <= 0)
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("%s: SkillNode '%s' has invalid DisciplinePointCost."),
					*GetName(), *Node.NodeName.ToString())));
			Result = EDataValidationResult::Invalid;
		}
	}

	// Warn if tiers are missing nodes
	if (Tier1Count == 0)
	{
		Context.AddError(FText::FromString(
			FString::Printf(TEXT("%s: No Tier 1 nodes defined."), *GetName())));
		Result = EDataValidationResult::Invalid;
	}

	if (Tier2Count == 0)
	{
		Context.AddError(FText::FromString(
			FString::Printf(TEXT("%s: No Tier 2 nodes defined."), *GetName())));
		Result = EDataValidationResult::Invalid;
	}

	if (Tier3Count == 0)
	{
		Context.AddError(FText::FromString(
			FString::Printf(TEXT("%s: No Tier 3 nodes defined."), *GetName())));
		Result = EDataValidationResult::Invalid;
	}

	// Stats
	if (!DisciplineAttributeModifierEffect)
	{
		Context.AddWarning(FText::FromString(
			FString::Printf(TEXT("%s: DisciplineAttributeModifierEffect is not set."), *GetName())));
	}

	return Result;
}

#endif