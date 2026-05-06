// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "SFNarrativeTags.h"

namespace SFNarrativeTags
{
    UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Root, "Narrative")

        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Quest_Root, "Narrative.Quest")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Quest_State_Started, "Narrative.Quest.State.Started")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Quest_State_Completed, "Narrative.Quest.State.Completed")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Quest_State_Failed, "Narrative.Quest.State.Failed")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Quest_State_Abandoned, "Narrative.Quest.State.Abandoned")

        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Quest_Task_Root, "Narrative.Quest.Task")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Quest_Task_Kill, "Narrative.Quest.Task.Kill")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Quest_Task_Collect, "Narrative.Quest.Task.Collect")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Quest_Task_Interact, "Narrative.Quest.Task.Interact")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Quest_Task_Travel, "Narrative.Quest.Task.Travel")

        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Dialogue_Root, "Narrative.Dialogue")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Dialogue_Event_Root, "Narrative.Dialogue.Event")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Dialogue_Event_Begin, "Narrative.Dialogue.Event.Begin")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Dialogue_Event_Advance, "Narrative.Dialogue.Event.Advance")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Dialogue_Event_End, "Narrative.Dialogue.Event.End")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Dialogue_Event_OptionChosen, "Narrative.Dialogue.Event.OptionChosen")

        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Dialogue_Memory_Root, "Narrative.Dialogue.Memory")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Dialogue_Memory_Promise, "Narrative.Dialogue.Memory.Promise")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Dialogue_Memory_Threat, "Narrative.Dialogue.Memory.Threat")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Dialogue_Memory_Betrayal, "Narrative.Dialogue.Memory.Betrayal")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Dialogue_Memory_Favor, "Narrative.Dialogue.Memory.Favor")

        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_World_Root, "Narrative.World")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_World_Fact_Root, "Narrative.World.Fact")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_World_Fact_LocationState, "Narrative.World.Fact.LocationState")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_World_Fact_Flag, "Narrative.World.Fact.Flag")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_World_Fact_Time, "Narrative.World.Fact.Time")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_World_Fact_CombatState, "Narrative.World.Fact.CombatState")

        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Faction_Root, "Narrative.Faction")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Faction_Standing_Root, "Narrative.Faction.StandingBand")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Faction_Standing_Hostile, "Narrative.Faction.StandingBand.Hostile")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Faction_Standing_Adversarial, "Narrative.Faction.StandingBand.Adversarial")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Faction_Standing_Wary, "Narrative.Faction.StandingBand.Wary")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Faction_Standing_Neutral, "Narrative.Faction.StandingBand.Neutral")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Faction_Standing_Cooperative, "Narrative.Faction.StandingBand.Cooperative")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Faction_Standing_Allied, "Narrative.Faction.StandingBand.Allied")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Faction_Standing_Bound, "Narrative.Faction.StandingBand.Bound")

        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Identity_Root, "Narrative.Identity")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Identity_Axis_Root, "Narrative.Identity.Axis")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Identity_Axis_Mercy, "Narrative.Identity.Axis.Mercy")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Identity_Axis_Truth, "Narrative.Identity.Axis.Truth")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Identity_Axis_Loyalty, "Narrative.Identity.Axis.Loyalty")

        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Outcome_Root, "Narrative.Outcome")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Ending_Root, "Narrative.Ending")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Ending_Availability_Hidden, "Narrative.Ending.Availability.Hidden")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Ending_Availability_Available, "Narrative.Ending.Availability.Available")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Ending_Availability_Threatened, "Narrative.Ending.Availability.Threatened")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Ending_Availability_Locked, "Narrative.Ending.Availability.Locked")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_Ending_Availability_Dominant, "Narrative.Ending.Availability.Dominant")

        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_System_Root, "Narrative.System")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_System_Debug, "Narrative.System.Debug")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_System_Checkpoint, "Narrative.System.Checkpoint")
        UE_DEFINE_GAMEPLAY_TAG(TAG_Narrative_System_Resync, "Narrative.System.Resync")
}

bool FSFNarrativeTagHelpers::IsNarrativeTag(const FGameplayTag& Tag)
{
    return Tag.MatchesTag(SFNarrativeTags::TAG_Narrative_Root);
}

bool FSFNarrativeTagHelpers::IsQuestTag(const FGameplayTag& Tag)
{
    return Tag.MatchesTag(SFNarrativeTags::TAG_Narrative_Quest_Root);
}

bool FSFNarrativeTagHelpers::IsDialogueTag(const FGameplayTag& Tag)
{
    return Tag.MatchesTag(SFNarrativeTags::TAG_Narrative_Dialogue_Root);
}

bool FSFNarrativeTagHelpers::IsWorldFactTag(const FGameplayTag& Tag)
{
    return Tag.MatchesTag(SFNarrativeTags::TAG_Narrative_World_Fact_Root);
}

bool FSFNarrativeTagHelpers::IsFactionTag(const FGameplayTag& Tag)
{
    return Tag.MatchesTag(SFNarrativeTags::TAG_Narrative_Faction_Root);
}

bool FSFNarrativeTagHelpers::IsIdentityTag(const FGameplayTag& Tag)
{
    return Tag.MatchesTag(SFNarrativeTags::TAG_Narrative_Identity_Root);
}

bool FSFNarrativeTagHelpers::IsEndingTag(const FGameplayTag& Tag)
{
    return Tag.MatchesTag(SFNarrativeTags::TAG_Narrative_Ending_Root);
}

FGameplayTag FSFNarrativeTagHelpers::GetStandingTagForBand(ESFFactionStandingBand Band)
{
    switch (Band)
    {
    case ESFFactionStandingBand::Hostile:     return SFNarrativeTags::TAG_Narrative_Faction_Standing_Hostile;
    case ESFFactionStandingBand::Adversarial: return SFNarrativeTags::TAG_Narrative_Faction_Standing_Adversarial;
    case ESFFactionStandingBand::Wary:        return SFNarrativeTags::TAG_Narrative_Faction_Standing_Wary;
    case ESFFactionStandingBand::Neutral:     return SFNarrativeTags::TAG_Narrative_Faction_Standing_Neutral;
    case ESFFactionStandingBand::Cooperative: return SFNarrativeTags::TAG_Narrative_Faction_Standing_Cooperative;
    case ESFFactionStandingBand::Allied:      return SFNarrativeTags::TAG_Narrative_Faction_Standing_Allied;
    case ESFFactionStandingBand::Bound:       return SFNarrativeTags::TAG_Narrative_Faction_Standing_Bound;
    case ESFFactionStandingBand::Unknown:
    default:                                  return SFNarrativeTags::TAG_Narrative_Faction_Standing_Root;
    }
}

ESFFactionStandingBand FSFNarrativeTagHelpers::GetBandForStandingTag(const FGameplayTag& Tag)
{
    if (Tag == SFNarrativeTags::TAG_Narrative_Faction_Standing_Hostile)     return ESFFactionStandingBand::Hostile;
    if (Tag == SFNarrativeTags::TAG_Narrative_Faction_Standing_Adversarial) return ESFFactionStandingBand::Adversarial;
    if (Tag == SFNarrativeTags::TAG_Narrative_Faction_Standing_Wary)        return ESFFactionStandingBand::Wary;
    if (Tag == SFNarrativeTags::TAG_Narrative_Faction_Standing_Neutral)     return ESFFactionStandingBand::Neutral;
    if (Tag == SFNarrativeTags::TAG_Narrative_Faction_Standing_Cooperative) return ESFFactionStandingBand::Cooperative;
    if (Tag == SFNarrativeTags::TAG_Narrative_Faction_Standing_Allied)      return ESFFactionStandingBand::Allied;
    if (Tag == SFNarrativeTags::TAG_Narrative_Faction_Standing_Bound)       return ESFFactionStandingBand::Bound;
    return ESFFactionStandingBand::Unknown;
}
