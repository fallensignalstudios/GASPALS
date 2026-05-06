// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
#include "SFNarrativeTypes.h"

/**
 * Primary log category for the Sovereign Call narrative system.
 *
 *   Verbose: high-frequency internal state and flow
 *   Log:     important state changes (quest start, dialogue enter, etc.)
 *   Warning: suspicious but recoverable behavior
 *   Error:   hard failures or corrupted data
 */
SIGNALFORGERPG_API DECLARE_LOG_CATEGORY_EXTERN(LogSFNarrative, Log, All);

/**
 * Enum to string helpers. Inline to avoid a TU dependency for log macros.
 */
namespace SFNarrativeLogHelpers
{
    inline const TCHAR* ToString(ESFQuestCompletionState State)
    {
        switch (State)
        {
        case ESFQuestCompletionState::NotStarted: return TEXT("NotStarted");
        case ESFQuestCompletionState::InProgress: return TEXT("InProgress");
        case ESFQuestCompletionState::Succeeded:  return TEXT("Succeeded");
        case ESFQuestCompletionState::Failed:     return TEXT("Failed");
        case ESFQuestCompletionState::Abandoned:  return TEXT("Abandoned");
        default:                                  return TEXT("Unknown");
        }
    }

    inline const TCHAR* ToString(ESFDialogueExitReason Reason)
    {
        switch (Reason)
        {
        case ESFDialogueExitReason::Completed:     return TEXT("Completed");
        case ESFDialogueExitReason::Cancelled:     return TEXT("Cancelled");
        case ESFDialogueExitReason::Interrupted:   return TEXT("Interrupted");
        case ESFDialogueExitReason::Replaced:      return TEXT("Replaced");
        case ESFDialogueExitReason::NetworkDesync: return TEXT("NetworkDesync");
        default:                                   return TEXT("Unknown");
        }
    }

    inline const TCHAR* ToString(ESFFactionStandingBand Band)
    {
        switch (Band)
        {
        case ESFFactionStandingBand::Hostile:     return TEXT("Hostile");
        case ESFFactionStandingBand::Adversarial: return TEXT("Adversarial");
        case ESFFactionStandingBand::Wary:        return TEXT("Wary");
        case ESFFactionStandingBand::Neutral:     return TEXT("Neutral");
        case ESFFactionStandingBand::Cooperative: return TEXT("Cooperative");
        case ESFFactionStandingBand::Allied:      return TEXT("Allied");
        case ESFFactionStandingBand::Bound:       return TEXT("Bound");
        case ESFFactionStandingBand::Unknown:
        default:                                  return TEXT("Unknown");
        }
    }

    inline const TCHAR* ToString(ESFEndingAvailability Availability)
    {
        switch (Availability)
        {
        case ESFEndingAvailability::Hidden:     return TEXT("Hidden");
        case ESFEndingAvailability::Available:  return TEXT("Available");
        case ESFEndingAvailability::Threatened: return TEXT("Threatened");
        case ESFEndingAvailability::Locked:     return TEXT("Locked");
        case ESFEndingAvailability::Dominant:   return TEXT("Dominant");
        case ESFEndingAvailability::Unknown:
        default:                                return TEXT("Unknown");
        }
    }

    inline const TCHAR* ToString(ESFIdentityDriftDirection Direction)
    {
        switch (Direction)
        {
        case ESFIdentityDriftDirection::Negative: return TEXT("Negative");
        case ESFIdentityDriftDirection::Positive: return TEXT("Positive");
        case ESFIdentityDriftDirection::None:
        default:                                  return TEXT("None");
        }
    }

    inline const TCHAR* ToString(ESFNarrativeDeltaType Type)
    {
        switch (Type)
        {
        case ESFNarrativeDeltaType::None:                  return TEXT("None");
        case ESFNarrativeDeltaType::StartQuest:            return TEXT("StartQuest");
        case ESFNarrativeDeltaType::RestartQuest:          return TEXT("RestartQuest");
        case ESFNarrativeDeltaType::AbandonQuest:          return TEXT("AbandonQuest");
        case ESFNarrativeDeltaType::EnterQuestState:       return TEXT("EnterQuestState");
        case ESFNarrativeDeltaType::CompleteQuestTask:     return TEXT("CompleteQuestTask");
        case ESFNarrativeDeltaType::TaskProgress:          return TEXT("TaskProgress");
        case ESFNarrativeDeltaType::BeginDialogue:         return TEXT("BeginDialogue");
        case ESFNarrativeDeltaType::ExitDialogue:          return TEXT("ExitDialogue");
        case ESFNarrativeDeltaType::AdvanceDialogue:       return TEXT("AdvanceDialogue");
        case ESFNarrativeDeltaType::SelectDialogueOption:  return TEXT("SelectDialogueOption");
        case ESFNarrativeDeltaType::TriggerDialogueEvent:  return TEXT("TriggerDialogueEvent");
        case ESFNarrativeDeltaType::SetWorldFact:          return TEXT("SetWorldFact");
        case ESFNarrativeDeltaType::AddWorldFactTag:       return TEXT("AddWorldFactTag");
        case ESFNarrativeDeltaType::RemoveWorldFactTag:    return TEXT("RemoveWorldFactTag");
        case ESFNarrativeDeltaType::ApplyFactionDelta:     return TEXT("ApplyFactionDelta");
        case ESFNarrativeDeltaType::SetFactionStandingBand:return TEXT("SetFactionStandingBand");
        case ESFNarrativeDeltaType::ApplyIdentityDelta:    return TEXT("ApplyIdentityDelta");
        case ESFNarrativeDeltaType::ApplyOutcome:          return TEXT("ApplyOutcome");
        case ESFNarrativeDeltaType::SetEndingAvailability: return TEXT("SetEndingAvailability");
        case ESFNarrativeDeltaType::ApplyEndingScoreDelta: return TEXT("ApplyEndingScoreDelta");
        case ESFNarrativeDeltaType::SaveCheckpointLoaded:  return TEXT("SaveCheckpointLoaded");
        case ESFNarrativeDeltaType::FullStateResync:       return TEXT("FullStateResync");
        default:                                           return TEXT("Unknown");
        }
    }
}

/**
 * Logging macros for narrative code. Each call is prefixed with the originating
 * function and line so logs are grep-friendly.
 *
 *     NARR_LOG(Warning, TEXT("Quest %s missing definition"), *QuestId.ToString());
 *     NARR_VLOG(TEXT("Delta[%d]: %s"), Delta.Sequence, SFNarrativeLogHelpers::ToString(Delta.Type));
 */
#define NARR_LOG(Verbosity, Format, ...) \
    UE_LOG(LogSFNarrative, Verbosity, TEXT("%s:%d | ") Format, TEXT(__FUNCTION__), __LINE__, ##__VA_ARGS__)

#define NARR_VLOG(Format, ...) \
    UE_LOG(LogSFNarrative, Verbose, TEXT("%s:%d | ") Format, TEXT(__FUNCTION__), __LINE__, ##__VA_ARGS__)

#define NARR_LOG_DISPLAY(Format, ...) NARR_LOG(Display, Format, ##__VA_ARGS__)
#define NARR_LOG_WARNING(Format, ...) NARR_LOG(Warning, Format, ##__VA_ARGS__)
#define NARR_LOG_ERROR(Format, ...)   NARR_LOG(Error,   Format, ##__VA_ARGS__)

 /**
  * One-shot dump of a full FSFNarrativeDelta for debugging. Use sparingly in tight loops.
  */
#define NARR_LOG_DELTA(Delta) \
    NARR_VLOG(TEXT("Delta[%d] Type=%s SubjectId=%s Arg0=%s Arg1=%s Tag0=%s Tag1=%s Int0=%d Int1=%d Float0=%.3f Float1=%.3f Bool0=%s"), \
        (Delta).Sequence, \
        SFNarrativeLogHelpers::ToString((Delta).Type), \
        *(Delta).SubjectId.ToString(), \
        *(Delta).Arg0.ToString(), \
        *(Delta).Arg1.ToString(), \
        *(Delta).Tag0.ToString(), \
        *(Delta).Tag1.ToString(), \
        (Delta).Int0, \
        (Delta).Int1, \
        (Delta).Float0, \
        (Delta).Float1, \
        (Delta).bBool0 ? TEXT("true") : TEXT("false"))

