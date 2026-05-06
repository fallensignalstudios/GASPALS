// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeFlagMap.generated.h"

/**
 * Lightweight, scoped collection of boolean narrative flags.
 *
 * Intended to be embedded in:
 *  - USFNarrativeComponent (per-owner flags),
 *  - quest instances / snapshots,
 *  - dialogue sessions,
 * or used directly by subsystems.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeFlagMap
{
    GENERATED_BODY()

public:
    /** Stored flags: true = set, false = explicitly cleared. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative|Flags")
    TMap<FName, bool> Flags;

    /** Optional metadata: tag classification per flag key (for tooling/UI). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative|Flags")
    TMap<FName, FGameplayTagContainer> FlagTagsByName;

public:
    //
    // Basic API
    //

    /** Set a flag to the given value (default true). */
    void SetFlag(FName FlagName, bool bValue = true)
    {
        if (FlagName.IsNone())
        {
            return;
        }
        Flags.FindOrAdd(FlagName) = bValue;
    }

    /** Set a flag to true. */
    void MarkFlagTrue(FName FlagName)
    {
        SetFlag(FlagName, true);
    }

    /** Set a flag to false (explicitly cleared). */
    void MarkFlagFalse(FName FlagName)
    {
        SetFlag(FlagName, false);
    }

    /** Remove a flag entirely (as if never set). */
    void ClearFlag(FName FlagName)
    {
        Flags.Remove(FlagName);
        FlagTagsByName.Remove(FlagName);
    }

    /** Remove all flags. */
    void Reset()
    {
        Flags.Reset();
        FlagTagsByName.Reset();
    }

    /** Query a flag. Returns false when missing unless bTreatMissingAsTrue is set. */
    bool GetFlag(FName FlagName, bool bTreatMissingAsTrue = false) const
    {
        if (FlagName.IsNone())
        {
            return false;
        }

        if (const bool* Found = Flags.Find(FlagName))
        {
            return *Found;
        }

        return bTreatMissingAsTrue;
    }

    /** True if the flag exists in the map (regardless of value). */
    bool HasFlag(FName FlagName) const
    {
        return Flags.Contains(FlagName);
    }

    //
    // Tagging / grouping
    //

    /** Attach classification tags to a flag (for tooling, filters, etc.). */
    void AddFlagTags(FName FlagName, const FGameplayTagContainer& Tags)
    {
        if (FlagName.IsNone() || Tags.Num() == 0)
        {
            return;
        }

        FGameplayTagContainer& Existing = FlagTagsByName.FindOrAdd(FlagName);
        Existing.AppendTags(Tags);
    }

    /** Get classification tags for a given flag. */
    void GetFlagTags(FName FlagName, FGameplayTagContainer& OutTags) const
    {
        OutTags.Reset();
        if (const FGameplayTagContainer* Found = FlagTagsByName.Find(FlagName))
        {
            OutTags = *Found;
        }
    }

    /** Collect all flag names that have the given tag. */
    void GetFlagsWithTag(FGameplayTag Tag, TArray<FName>& OutFlagNames) const
    {
        OutFlagNames.Reset();

        if (!Tag.IsValid())
        {
            return;
        }

        for (const TPair<FName, FGameplayTagContainer>& Pair : FlagTagsByName)
        {
            if (Pair.Value.HasTag(Tag))
            {
                OutFlagNames.Add(Pair.Key);
            }
        }
    }

    //
    // Bulk queries
    //

    /** Are ALL of these flags true (missing flags treated as `bTreatMissingAsTrue`)? */
    bool AreAllFlagsTrue(const TArray<FName>& FlagNames, bool bTreatMissingAsTrue = false) const
    {
        for (const FName& Name : FlagNames)
        {
            if (!GetFlag(Name, bTreatMissingAsTrue))
            {
                return false;
            }
        }
        return true;
    }

    /** Is AT LEAST ONE of these flags true (missing flags treated as `bTreatMissingAsTrue`)? */
    bool IsAnyFlagTrue(const TArray<FName>& FlagNames, bool bTreatMissingAsTrue = false) const
    {
        for (const FName& Name : FlagNames)
        {
            if (GetFlag(Name, bTreatMissingAsTrue))
            {
                return true;
            }
        }
        return false;
    }

    //
    // Serialization helpers (for embedding into save data)
    //

    /** Serialize flags into a simple array of key/value pairs for save data. */
    void BuildSnapshot(TArray<FSFWorldFactSnapshot>& OutSnapshots, const FGameplayTag& FactTagBase, FName ContextIdBase = NAME_None) const
    {
        auto MakeFlagContext = [ContextIdBase](FName FlagName)
        {
            if (FlagName.IsNone())
            {
                return ContextIdBase;
            }

            if (ContextIdBase.IsNone())
            {
                return FlagName;
            }

            return FName(*FString::Printf(TEXT("%s|%s"), *ContextIdBase.ToString(), *FlagName.ToString()));
        };

        OutSnapshots.Reset();
        OutSnapshots.Reserve(Flags.Num());

        for (const TPair<FName, bool>& Pair : Flags)
        {
            FSFWorldFactSnapshot Snapshot;
            Snapshot.Key.FactTag = FactTagBase;
            Snapshot.Key.ContextId = MakeFlagContext(Pair.Key);
            Snapshot.Value = FSFWorldFactValue::MakeBool(Pair.Value);
            OutSnapshots.Add(MoveTemp(Snapshot));
        }
    }

    /** Restore flags from snapshots that share a base fact tag/context. */
    void RestoreFromSnapshots(const TArray<FSFWorldFactSnapshot>& Snapshots, const FGameplayTag& FactTagBase, FName ContextIdBase = NAME_None)
    {
        auto ExtractFlagName = [ContextIdBase](FName EncodedContextId, FName& OutFlagName)
        {
            OutFlagName = NAME_None;

            if (EncodedContextId.IsNone())
            {
                return false;
            }

            const FString Encoded = EncodedContextId.ToString();
            if (ContextIdBase.IsNone())
            {
                OutFlagName = FName(*Encoded);
                return !OutFlagName.IsNone();
            }

            const FString Prefix = ContextIdBase.ToString() + TEXT("|");
            if (!Encoded.StartsWith(Prefix))
            {
                return false;
            }

            const FString Suffix = Encoded.RightChop(Prefix.Len());
            if (Suffix.IsEmpty())
            {
                return false;
            }

            OutFlagName = FName(*Suffix);
            return !OutFlagName.IsNone();
        };

        Reset();

        for (const FSFWorldFactSnapshot& Snapshot : Snapshots)
        {
            if (Snapshot.Key.FactTag != FactTagBase)
            {
                continue;
            }

            if (Snapshot.Value.ValueType != ESFNarrativeFactValueType::Bool)
            {
                continue;
            }

            FName FlagName = NAME_None;
            if (!ExtractFlagName(Snapshot.Key.ContextId, FlagName))
            {
                continue;
            }

            Flags.Add(FlagName, Snapshot.Value.BoolValue);
        }
    }
};
