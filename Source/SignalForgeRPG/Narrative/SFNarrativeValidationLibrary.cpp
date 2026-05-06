#include "SFNarrativeValidationLibrary.h"

#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"

#include "Dialogue/Data/SFDialogueComponent.h"
#include "SFNarrativeComponent.h"
#include "SFNarrativeReplicatorComponent.h"
#include "SFQuestDefinition.h"

namespace SFNarrativeValidation
{
    static bool IsConditionSetEmpty(const FSFNarrativeConditionSet& ConditionSet)
    {
        return ConditionSet.AllConditions.Num() == 0
            && ConditionSet.AnyConditions.Num() == 0
            && ConditionSet.NoneConditions.Num() == 0;
    }

    static bool IsDeltaObviouslyEmpty(const FSFNarrativeDelta& Delta)
    {
        return Delta.Type == ESFNarrativeDeltaType::None;
    }

    static FText MakeIndexedMessage(const TCHAR* Prefix, int32 Index, const FText& Inner)
    {
        return FText::FromString(FString::Printf(TEXT("%s[%d]: %s"), Prefix, Index, *Inner.ToString()));
    }
}

FSFNarrativeValidationResult USFNarrativeValidationLibrary::ValidateQuestDefinition(USFQuestDefinition* QuestDef)
{
    FSFNarrativeValidationResult Result;

    if (!QuestDef)
    {
        Result.AddError(FText::FromString(TEXT("Quest definition is null.")));
        return Result;
    }

    const FPrimaryAssetId QuestId = QuestDef->GetPrimaryAssetId();

    if (QuestDef->QuestTags.Num() == 0)
    {
        Result.AddWarning(
            FText::FromString(FString::Printf(TEXT("Quest '%s' has no QuestTags set."), *QuestId.ToString())),
            QuestDef);
    }

    if (!QuestDef->HasAnyStates())
    {
        Result.AddError(
            FText::FromString(FString::Printf(TEXT("Quest '%s' defines no states."), *QuestId.ToString())),
            QuestDef);
    }
    else if (!QuestDef->HasValidStartState())
    {
        Result.AddError(
            FText::FromString(FString::Printf(TEXT("Quest '%s' has no valid start state."), *QuestId.ToString())),
            QuestDef);
    }

    TArray<FText> TransitionIssues;
    QuestDef->ValidateTransitions(TransitionIssues);
    for (const FText& Issue : TransitionIssues)
    {
        Result.AddError(Issue, QuestDef);
    }

    TArray<FText> TaskIssues;
    QuestDef->ValidateTasks(TaskIssues);
    for (const FText& Issue : TaskIssues)
    {
        Result.AddError(Issue, QuestDef);
    }

    return Result;
}

FSFNarrativeValidationResult USFNarrativeValidationLibrary::ValidateQuestDatabase(USFQuestDatabase* QuestDatabase)
{
    FSFNarrativeValidationResult Result;

    if (!QuestDatabase)
    {
        Result.AddError(FText::FromString(TEXT("Quest database is null.")));
        return Result;
    }

    TSet<FPrimaryAssetId> SeenIds;

    for (const TSoftObjectPtr<USFQuestDefinition>& SoftQuest : QuestDatabase->AllQuests)
    {
        USFQuestDefinition* Quest = SoftQuest.LoadSynchronous();
        if (!Quest)
        {
            Result.AddWarning(
                FText::FromString(FString::Printf(TEXT("Quest entry %s failed to load."), *SoftQuest.ToString())),
                QuestDatabase);
            continue;
        }

        const FPrimaryAssetId QuestId = Quest->GetPrimaryAssetId();
        if (SeenIds.Contains(QuestId))
        {
            Result.AddError(
                FText::FromString(FString::Printf(TEXT("Duplicate quest id '%s' in quest database."), *QuestId.ToString())),
                QuestDatabase,
                QuestId.PrimaryAssetName);
        }
        else
        {
            SeenIds.Add(QuestId);
        }

        Result.Merge(ValidateQuestDefinition(Quest));
    }

    return Result;
}

FSFNarrativeValidationResult USFNarrativeValidationLibrary::ValidateConditionSet(const FSFNarrativeConditionSet& ConditionSet)
{
    FSFNarrativeValidationResult Result;

    auto ValidateArray = [&Result](const TArray<FSFNarrativeCondition>& Conditions, const TCHAR* GroupName)
        {
            for (int32 Index = 0; Index < Conditions.Num(); ++Index)
            {
                const FSFNarrativeCondition& C = Conditions[Index];

                if (C.Domain == ESFNarrativeConditionDomain::None)
                {
                    Result.AddWarning(
                        FText::FromString(FString::Printf(
                            TEXT("ConditionSet has condition with Domain=None in group '%s' at index %d."),
                            GroupName, Index)));
                    continue;
                }

                switch (C.Domain)
                {
                case ESFNarrativeConditionDomain::WorldFact:
                    if (!C.WorldFact.Key.FactTag.IsValid())
                    {
                        Result.AddError(
                            FText::FromString(FString::Printf(
                                TEXT("WorldFact condition in group '%s' at index %d has invalid FactTag."),
                                GroupName, Index)));
                    }
                    break;

                case ESFNarrativeConditionDomain::Faction:
                    if (!C.Faction.FactionTag.IsValid())
                    {
                        Result.AddError(
                            FText::FromString(FString::Printf(
                                TEXT("Faction condition in group '%s' at index %d has invalid FactionTag."),
                                GroupName, Index)));
                    }
                    break;

                case ESFNarrativeConditionDomain::IdentityAxis:
                    if (!C.IdentityAxis.AxisTag.IsValid())
                    {
                        Result.AddError(
                            FText::FromString(FString::Printf(
                                TEXT("IdentityAxis condition in group '%s' at index %d has invalid AxisTag."),
                                GroupName, Index)));
                    }
                    break;

                case ESFNarrativeConditionDomain::QuestState:
                    if (C.QuestState.RequiredStateId.IsNone()
                        && C.QuestState.RequiredCompletionState == ESFQuestCompletionState::NotStarted)
                    {
                        Result.AddWarning(
                            FText::FromString(FString::Printf(
                                TEXT("QuestState condition in group '%s' at index %d has neither RequiredStateId nor non-default CompletionState set."),
                                GroupName, Index)));
                    }
                    break;

                default:
                    break;
                }
            }
        };

    ValidateArray(ConditionSet.AllConditions, TEXT("All"));
    ValidateArray(ConditionSet.AnyConditions, TEXT("Any"));
    ValidateArray(ConditionSet.NoneConditions, TEXT("None"));

    if (SFNarrativeValidation::IsConditionSetEmpty(ConditionSet))
    {
        Result.AddInfo(FText::FromString(TEXT("ConditionSet is empty - always true.")));
    }

    return Result;
}

FSFNarrativeValidationResult USFNarrativeValidationLibrary::ValidateNarrativeDelta(const FSFNarrativeDelta& Delta)
{
    FSFNarrativeValidationResult Result;

    if (SFNarrativeValidation::IsDeltaObviouslyEmpty(Delta))
    {
        Result.AddWarning(FText::FromString(TEXT("Narrative delta has Type=None and will do nothing.")));
        return Result;
    }

    switch (Delta.Type)
    {
    case ESFNarrativeDeltaType::SetWorldFact:
        if (!Delta.Tag0.IsValid())
        {
            Result.AddError(FText::FromString(TEXT("SetWorldFact delta is missing Tag0 / FactTag.")));
        }
        break;

    case ESFNarrativeDeltaType::ApplyFactionDelta:
        if (!Delta.Tag0.IsValid())
        {
            Result.AddError(FText::FromString(TEXT("ApplyFactionDelta delta is missing Tag0 / FactionTag.")));
        }
        break;

    case ESFNarrativeDeltaType::ApplyIdentityDelta:
        if (!Delta.Tag0.IsValid())
        {
            Result.AddError(FText::FromString(TEXT("ApplyIdentityDelta delta is missing Tag0 / AxisTag.")));
        }
        break;

    case ESFNarrativeDeltaType::SetEndingAvailability:
        if (!Delta.Tag0.IsValid())
        {
            Result.AddError(FText::FromString(TEXT("SetEndingAvailability delta is missing Tag0 / EndingTag.")));
        }
        break;

    case ESFNarrativeDeltaType::StartQuest:
    case ESFNarrativeDeltaType::RestartQuest:
    case ESFNarrativeDeltaType::AbandonQuest:
    case ESFNarrativeDeltaType::EnterQuestState:
    case ESFNarrativeDeltaType::TaskProgress:
        if (Delta.SubjectId.IsNone())
        {
            Result.AddWarning(
                FText::FromString(TEXT("Quest-related delta has no SubjectId; expected a quest identifier or asset name.")));
        }
        break;

    case ESFNarrativeDeltaType::ApplyOutcome:
        if (Delta.SubjectId.IsNone())
        {
            Result.AddWarning(
                FText::FromString(TEXT("ApplyOutcome delta has no SubjectId; expected an outcome identifier.")));
        }
        break;

    default:
        break;
    }

    return Result;
}

FSFNarrativeValidationResult USFNarrativeValidationLibrary::ValidateNarrativeDeltas(const TArray<FSFNarrativeDelta>& Deltas)
{
    FSFNarrativeValidationResult Result;

    if (Deltas.Num() == 0)
    {
        Result.AddInfo(FText::FromString(TEXT("Delta array is empty.")));
        return Result;
    }

    for (int32 Index = 0; Index < Deltas.Num(); ++Index)
    {
        FSFNarrativeValidationResult DeltaResult = ValidateNarrativeDelta(Deltas[Index]);

        for (FSFNarrativeValidationMessage& Msg : DeltaResult.Messages)
        {
            Msg.Message = SFNarrativeValidation::MakeIndexedMessage(TEXT("Delta"), Index, Msg.Message);
        }

        Result.Merge(DeltaResult);
    }

    return Result;
}

FSFNarrativeValidationResult USFNarrativeValidationLibrary::ValidateEventTable(UDataTable* EventTable)
{
    FSFNarrativeValidationResult Result;

    if (!EventTable)
    {
        Result.AddError(FText::FromString(TEXT("Event table is null.")));
        return Result;
    }

    if (!EventTable->GetRowStruct() || !EventTable->GetRowStruct()->IsChildOf(FSFNarrativeEventTableRow::StaticStruct()))
    {
        Result.AddError(
            FText::FromString(TEXT("Event table uses the wrong row struct (expected FSFNarrativeEventTableRow).")),
            EventTable);
        return Result;
    }

    TSet<FName> SeenEventIds;

    for (const TPair<FName, uint8*>& Pair : EventTable->GetRowMap())
    {
        const FName RowName = Pair.Key;
        const FSFNarrativeEventTableRow* Row = reinterpret_cast<const FSFNarrativeEventTableRow*>(Pair.Value);

        if (!Row)
        {
            continue;
        }

        const FName EventId = Row->EventId.IsNone() ? RowName : Row->EventId;

        if (SeenEventIds.Contains(EventId))
        {
            Result.AddError(
                FText::FromString(FString::Printf(TEXT("Duplicate EventId '%s' in narrative event table."), *EventId.ToString())),
                EventTable,
                EventId);
        }
        else
        {
            SeenEventIds.Add(EventId);
        }

        if (!Row->EventCategory.IsValid())
        {
            Result.AddWarning(
                FText::FromString(FString::Printf(TEXT("Event '%s' has no EventCategory set."), *EventId.ToString())),
                EventTable,
                EventId);
        }

        const FSFNarrativeValidationResult ConditionRes = ValidateConditionSet(Row->EligibilityConditions);
        for (FSFNarrativeValidationMessage Msg : ConditionRes.Messages)
        {
            Msg.Asset = EventTable;
            Msg.SubjectId = EventId;
            Result.Messages.Add(Msg);
        }
        Result.ErrorCount += ConditionRes.ErrorCount;
        Result.WarningCount += ConditionRes.WarningCount;

        if (!Row->bRepeatable && Row->CooldownSeconds > 0.0f && !Row->FiredFactKey.FactTag.IsValid())
        {
            Result.AddWarning(
                FText::FromString(FString::Printf(
                    TEXT("Event '%s' is non-repeatable with cooldown but has no FiredFactKey; it may be selected again after cooldown."),
                    *EventId.ToString())),
                EventTable,
                EventId);
        }

        if (!Row->FiredFactKey.FactTag.IsValid() && !Row->bRepeatable)
        {
            Result.AddInfo(
                FText::FromString(FString::Printf(
                    TEXT("Event '%s' does not define a FiredFactKey. This is valid, but it relies on external tracking if it should not fire twice."),
                    *EventId.ToString())),
                EventTable,
                EventId);
        }

        // If your row has consequence deltas, enable this block and rename the field if needed.
        // Result.Merge(ValidateNarrativeDeltas(Row->Consequences));
    }

    return Result;
}

FSFNarrativeValidationResult USFNarrativeValidationLibrary::ValidateDialogueNarrativeMap(USFDialogueNarrativeMap* MapAsset)
{
    FSFNarrativeValidationResult Result;

    if (!MapAsset)
    {
        Result.AddError(FText::FromString(TEXT("DialogueNarrativeMap is null.")));
        return Result;
    }

    TSet<FSFDialogueKey> SeenKeys;

    for (const FSFDialogueNarrativeEntry& Entry : MapAsset->Entries)
    {
        if (!Entry.Key.IsValid())
        {
            Result.AddError(
                FText::FromString(TEXT("Dialogue entry has invalid Key (ConversationId/NodeId missing).")),
                MapAsset);
            continue;
        }

        if (SeenKeys.Contains(Entry.Key))
        {
            Result.AddError(
                FText::FromString(FString::Printf(
                    TEXT("Duplicate dialogue narrative entry for Conversation='%s', Node='%s', Option=%d."),
                    *Entry.Key.ConversationId.ToString(),
                    *Entry.Key.NodeId.ToString(),
                    Entry.Key.OptionIndex)),
                MapAsset,
                Entry.Key.NodeId);
        }
        else
        {
            SeenKeys.Add(Entry.Key);
        }

        if (Entry.OnEnterDeltas.Num() == 0
            && Entry.OnExitDeltas.Num() == 0
            && SFNarrativeValidation::IsConditionSetEmpty(Entry.AvailabilityConditions))
        {
            Result.AddWarning(
                FText::FromString(FString::Printf(
                    TEXT("Dialogue entry for Conversation='%s', Node='%s', Option=%d has no conditions or deltas; it may not need a narrative mapping."),
                    *Entry.Key.ConversationId.ToString(),
                    *Entry.Key.NodeId.ToString(),
                    Entry.Key.OptionIndex)),
                MapAsset,
                Entry.Key.NodeId);
        }

        if (Entry.bMarkAsSeenFact && !Entry.SeenFactKey.FactTag.IsValid())
        {
            Result.AddError(
                FText::FromString(FString::Printf(
                    TEXT("Dialogue entry for Conversation='%s', Node='%s', Option=%d marks itself as seen but SeenFactKey is invalid."),
                    *Entry.Key.ConversationId.ToString(),
                    *Entry.Key.NodeId.ToString(),
                    Entry.Key.OptionIndex)),
                MapAsset,
                Entry.Key.NodeId);
        }

        const FSFNarrativeValidationResult CondRes = ValidateConditionSet(Entry.AvailabilityConditions);
        for (FSFNarrativeValidationMessage Msg : CondRes.Messages)
        {
            Msg.Asset = MapAsset;
            Msg.SubjectId = Entry.Key.NodeId;
            Result.Messages.Add(Msg);
        }
        Result.ErrorCount += CondRes.ErrorCount;
        Result.WarningCount += CondRes.WarningCount;

        FSFNarrativeValidationResult EnterDeltaRes = ValidateNarrativeDeltas(Entry.OnEnterDeltas);
        for (FSFNarrativeValidationMessage& Msg : EnterDeltaRes.Messages)
        {
            Msg.Asset = MapAsset;
            Msg.SubjectId = Entry.Key.NodeId;
            Msg.Message = FText::FromString(FString::Printf(TEXT("[OnEnter] %s"), *Msg.Message.ToString()));
        }
        Result.Merge(EnterDeltaRes);

        FSFNarrativeValidationResult ExitDeltaRes = ValidateNarrativeDeltas(Entry.OnExitDeltas);
        for (FSFNarrativeValidationMessage& Msg : ExitDeltaRes.Messages)
        {
            Msg.Asset = MapAsset;
            Msg.SubjectId = Entry.Key.NodeId;
            Msg.Message = FText::FromString(FString::Printf(TEXT("[OnExit] %s"), *Msg.Message.ToString()));
        }
        Result.Merge(ExitDeltaRes);
    }

    return Result;
}

FSFNarrativeValidationResult USFNarrativeValidationLibrary::ValidateNarrativeComponentConfiguration(AActor* OwnerActor)
{
    FSFNarrativeValidationResult Result;

    if (!OwnerActor)
    {
        Result.AddError(FText::FromString(TEXT("OwnerActor is null.")));
        return Result;
    }

    USFNarrativeComponent* NarrativeComponent = OwnerActor->FindComponentByClass<USFNarrativeComponent>();
    if (!NarrativeComponent)
    {
        Result.AddError(
            FText::FromString(TEXT("OwnerActor does not have a USFNarrativeComponent.")),
            OwnerActor);
        return Result;
    }

    USFDialogueComponent* DialogueComponent = OwnerActor->FindComponentByClass<USFDialogueComponent>();
    if (!DialogueComponent)
    {
        Result.AddWarning(
            FText::FromString(TEXT("OwnerActor has a USFNarrativeComponent but no USFDialogueComponent. This is fine if dialogue is not used on this actor.")),
            OwnerActor);
    }

    USFNarrativeReplicatorComponent* Replicator = OwnerActor->FindComponentByClass<USFNarrativeReplicatorComponent>();
    if (OwnerActor->HasAuthority() && !Replicator)
    {
        Result.AddWarning(
            FText::FromString(TEXT("Authoritative actor has a USFNarrativeComponent but no USFNarrativeReplicatorComponent found. If runtime-created, this warning can be ignored; otherwise replication may be incomplete.")),
            OwnerActor);
    }

    // Legacy-runtime smell checks based on the older ownership model.
    const bool bHasLegacyWorldRuntime = OwnerActor->FindComponentByClass<UActorComponent>() && false;
    const bool bHasLegacyFactionRuntime = OwnerActor->FindComponentByClass<UActorComponent>() && false;
    const bool bHasLegacyIdentityRuntime = OwnerActor->FindComponentByClass<UActorComponent>() && false;
    const bool bHasLegacyOutcomeRuntime = OwnerActor->FindComponentByClass<UActorComponent>() && false;
    const bool bHasLegacyEndingRuntime = OwnerActor->FindComponentByClass<UActorComponent>() && false;

    if (bHasLegacyWorldRuntime || bHasLegacyFactionRuntime || bHasLegacyIdentityRuntime || bHasLegacyOutcomeRuntime || bHasLegacyEndingRuntime)
    {
        Result.AddWarning(
            FText::FromString(TEXT("OwnerActor appears to still use legacy runtime ownership alongside the newer narrative architecture. Check for duplicated state authority.")),
            OwnerActor);
    }

    return Result;
}

FSFNarrativeValidationResult USFNarrativeValidationLibrary::ValidateNarrativeSaveData(const FSFNarrativeSaveData& SaveData)
{
    FSFNarrativeValidationResult Result;

    // These checks are intentionally conservative because your exact save struct
    // may still evolve. Rename fields below to match your concrete struct.

    if (SaveData.SaveVersion <= 0)
    {
        Result.AddWarning(FText::FromString(TEXT("SaveData has a non-positive SaveVersion.")));
    }

    if (SaveData.ActiveQuestIds.Num() == 0
        && SaveData.CompletedQuestIds.Num() == 0
        && SaveData.WorldFacts.Num() == 0)
    {
        Result.AddInfo(FText::FromString(TEXT("SaveData appears empty (no quests or world facts captured).")));
    }

    TSet<FName> SeenQuestIds;
    for (const FName& QuestId : SaveData.ActiveQuestIds)
    {
        if (QuestId.IsNone())
        {
            Result.AddError(FText::FromString(TEXT("SaveData.ActiveQuestIds contains NAME_None.")));
            continue;
        }

        if (SeenQuestIds.Contains(QuestId))
        {
            Result.AddWarning(
                FText::FromString(FString::Printf(TEXT("SaveData.ActiveQuestIds contains duplicate quest id '%s'."), *QuestId.ToString())),
                nullptr,
                QuestId);
        }
        else
        {
            SeenQuestIds.Add(QuestId);
        }
    }

    for (const FName& QuestId : SaveData.CompletedQuestIds)
    {
        if (QuestId.IsNone())
        {
            Result.AddError(FText::FromString(TEXT("SaveData.CompletedQuestIds contains NAME_None.")));
        }
    }

    return Result;
}

FSFNarrativeValidationResult USFNarrativeValidationLibrary::ValidateNarrativeProject(
    USFQuestDatabase* QuestDatabase,
    USFDialogueNarrativeMap* DialogueMap,
    UDataTable* EventTable,
    AActor* ExampleOwnerActor)
{
    FSFNarrativeValidationResult Result;

    if (QuestDatabase)
    {
        Result.Merge(ValidateQuestDatabase(QuestDatabase));
    }
    else
    {
        Result.AddInfo(FText::FromString(TEXT("QuestDatabase was null; quest database validation skipped.")));
    }

    if (DialogueMap)
    {
        Result.Merge(ValidateDialogueNarrativeMap(DialogueMap));
    }
    else
    {
        Result.AddInfo(FText::FromString(TEXT("DialogueMap was null; dialogue narrative map validation skipped.")));
    }

    if (EventTable)
    {
        Result.Merge(ValidateEventTable(EventTable));
    }
    else
    {
        Result.AddInfo(FText::FromString(TEXT("EventTable was null; event table validation skipped.")));
    }

    if (ExampleOwnerActor)
    {
        Result.Merge(ValidateNarrativeComponentConfiguration(ExampleOwnerActor));
    }
    else
    {
        Result.AddInfo(FText::FromString(TEXT("ExampleOwnerActor was null; component configuration validation skipped.")));
    }

    return Result;
}