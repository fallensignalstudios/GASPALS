// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "SFNarrativeTypes.h" // for ESFFactionStandingBand

/**
 * Native gameplay tag declarations for the Sovereign Call narrative system.
 *
 * These tags are registered automatically by Unreal at module startup; you do
 * not need to add them to the project's GameplayTags ini (though you may, if
 * you want them visible/editable in the Tag Manager UI from boot).
 *
 * To reference a tag from code:
 *     SFNarrativeTags::TAG_Narrative_Quest_Root.GetTag()
 *
 * To reference a tag in Blueprint pickers, use the literal name string
 * ("Narrative.Quest", etc.) or expose via your project's tag config.
 */
namespace SFNarrativeTags
{
    // Root
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Root);

    // Quest
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Quest_Root);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Quest_State_Started);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Quest_State_Completed);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Quest_State_Failed);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Quest_State_Abandoned);

    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Quest_Task_Root);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Quest_Task_Kill);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Quest_Task_Collect);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Quest_Task_Interact);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Quest_Task_Travel);

    // Dialogue
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Dialogue_Root);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Dialogue_Event_Root);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Dialogue_Event_Begin);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Dialogue_Event_Advance);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Dialogue_Event_End);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Dialogue_Event_OptionChosen);

    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Dialogue_Memory_Root);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Dialogue_Memory_Promise);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Dialogue_Memory_Threat);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Dialogue_Memory_Betrayal);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Dialogue_Memory_Favor);

    // World facts
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_World_Root);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_World_Fact_Root);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_World_Fact_LocationState);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_World_Fact_Flag);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_World_Fact_Time);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_World_Fact_CombatState);

    // Factions
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Faction_Root);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Faction_Standing_Root);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Faction_Standing_Hostile);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Faction_Standing_Adversarial);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Faction_Standing_Wary);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Faction_Standing_Neutral);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Faction_Standing_Cooperative);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Faction_Standing_Allied);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Faction_Standing_Bound);

    // Identity
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Identity_Root);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Identity_Axis_Root);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Identity_Axis_Mercy);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Identity_Axis_Truth);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Identity_Axis_Loyalty);

    // Outcomes / endings
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Outcome_Root);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Ending_Root);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Ending_Availability_Hidden);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Ending_Availability_Available);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Ending_Availability_Threatened);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Ending_Availability_Locked);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_Ending_Availability_Dominant);

    // System
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_System_Root);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_System_Debug);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_System_Checkpoint);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Narrative_System_Resync);
}

/**
 * Static helpers for narrative tag classification and band/tag mapping.
 */
class SIGNALFORGERPG_API FSFNarrativeTagHelpers
{
public:
    /** True if Tag is anywhere under "Narrative". */
    static bool IsNarrativeTag(const FGameplayTag& Tag);
    static bool IsQuestTag(const FGameplayTag& Tag);
    static bool IsDialogueTag(const FGameplayTag& Tag);
    static bool IsWorldFactTag(const FGameplayTag& Tag);
    static bool IsFactionTag(const FGameplayTag& Tag);
    static bool IsIdentityTag(const FGameplayTag& Tag);
    static bool IsEndingTag(const FGameplayTag& Tag);

    /** Map a standing band enum to its corresponding tag. Returns the Standing root tag if Unknown/invalid. */
    static FGameplayTag GetStandingTagForBand(ESFFactionStandingBand Band);

    /** Inverse of GetStandingTagForBand; returns Unknown if Tag is not a standing band tag. */
    static ESFFactionStandingBand GetBandForStandingTag(const FGameplayTag& Tag);
};