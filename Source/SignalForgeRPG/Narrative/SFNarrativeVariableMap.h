#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeVariableMap.generated.h"

/**
 * Supported scalar variable types for FSFNarrativeVariableMap.
 */
UENUM(BlueprintType)
enum class ESFNarrativeVariableType : uint8
{
    None,
    Int,
    Float,
    Bool,
    Name,
    Tag,
    TagContainer
};

/**
 * Single variable value used by FSFNarrativeVariableMap.
 * This is intentionally very close to FSFWorldFactValue, but scoped to
 * owner-level/session-level variables instead of world facts.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeVariableValue
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Variables")
    ESFNarrativeVariableType Type = ESFNarrativeVariableType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Variables")
    int32 IntValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Variables")
    float FloatValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Variables")
    bool BoolValue = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Variables")
    FName NameValue = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Variables")
    FGameplayTag TagValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Variables")
    FGameplayTagContainer TagContainerValue;

    void Reset()
    {
        Type = ESFNarrativeVariableType::None;
        IntValue = 0;
        FloatValue = 0.0f;
        BoolValue = false;
        NameValue = NAME_None;
        TagValue = FGameplayTag();
        TagContainerValue.Reset();
    }
};

/**
 * Scoped map of named narrative variables.
 *
 * Intended to be embedded in:
 *  - USFNarrativeComponent (per-owner variables),
 *  - quest snapshots,
 *  - dialogue state snapshots (session variables),
 *  - or any other narrative container that needs light-weight key/value state.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeVariableMap
{
    GENERATED_BODY()

public:
    /** Map from variable name to value. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative|Variables")
    TMap<FName, FSFNarrativeVariableValue> Variables;

public:
    //
    // Core API
    //

    UFUNCTION(BlueprintCallable, Category = "Narrative|Variables")
    void Reset()
    {
        Variables.Reset();
    }

    UFUNCTION(BlueprintCallable, Category = "Narrative|Variables")
    void ClearVariable(FName Name)
    {
        Variables.Remove(Name);
    }

    UFUNCTION(BlueprintPure, Category = "Narrative|Variables")
    bool HasVariable(FName Name) const
    {
        return Variables.Contains(Name);
    }

    UFUNCTION(BlueprintPure, Category = "Narrative|Variables")
    ESFNarrativeVariableType GetVariableType(FName Name) const
    {
        if (const FSFNarrativeVariableValue* Found = Variables.Find(Name))
        {
            return Found->Type;
        }
        return ESFNarrativeVariableType::None;
    }

    //
    // Setters
    //

    UFUNCTION(BlueprintCallable, Category = "Narrative|Variables")
    void SetInt(FName Name, int32 Value)
    {
        if (Name.IsNone()) return;
        FSFNarrativeVariableValue& Var = Variables.FindOrAdd(Name);
        Var.Reset();
        Var.Type = ESFNarrativeVariableType::Int;
        Var.IntValue = Value;
    }

    UFUNCTION(BlueprintCallable, Category = "Narrative|Variables")
    void SetFloat(FName Name, float Value)
    {
        if (Name.IsNone()) return;
        FSFNarrativeVariableValue& Var = Variables.FindOrAdd(Name);
        Var.Reset();
        Var.Type = ESFNarrativeVariableType::Float;
        Var.FloatValue = Value;
    }

    UFUNCTION(BlueprintCallable, Category = "Narrative|Variables")
    void SetBool(FName Name, bool bValue)
    {
        if (Name.IsNone()) return;
        FSFNarrativeVariableValue& Var = Variables.FindOrAdd(Name);
        Var.Reset();
        Var.Type = ESFNarrativeVariableType::Bool;
        Var.BoolValue = bValue;
    }

    UFUNCTION(BlueprintCallable, Category = "Narrative|Variables")
    void SetName(FName Name, FName Value)
    {
        if (Name.IsNone()) return;
        FSFNarrativeVariableValue& Var = Variables.FindOrAdd(Name);
        Var.Reset();
        Var.Type = ESFNarrativeVariableType::Name;
        Var.NameValue = Value;
    }

    UFUNCTION(BlueprintCallable, Category = "Narrative|Variables")
    void SetTag(FName Name, FGameplayTag Tag)
    {
        if (Name.IsNone()) return;
        FSFNarrativeVariableValue& Var = Variables.FindOrAdd(Name);
        Var.Reset();
        Var.Type = ESFNarrativeVariableType::Tag;
        Var.TagValue = Tag;
    }

    UFUNCTION(BlueprintCallable, Category = "Narrative|Variables")
    void SetTagContainer(FName Name, const FGameplayTagContainer& Tags)
    {
        if (Name.IsNone()) return;
        FSFNarrativeVariableValue& Var = Variables.FindOrAdd(Name);
        Var.Reset();
        Var.Type = ESFNarrativeVariableType::TagContainer;
        Var.TagContainerValue = Tags;
    }

    //
    // Getters (return success + value)
    //

    UFUNCTION(BlueprintPure, Category = "Narrative|Variables")
    bool GetInt(FName Name, int32& OutValue) const
    {
        if (const FSFNarrativeVariableValue* Var = Variables.Find(Name))
        {
            if (Var->Type == ESFNarrativeVariableType::Int)
            {
                OutValue = Var->IntValue;
                return true;
            }
        }
        OutValue = 0;
        return false;
    }

    UFUNCTION(BlueprintPure, Category = "Narrative|Variables")
    bool GetFloat(FName Name, float& OutValue) const
    {
        if (const FSFNarrativeVariableValue* Var = Variables.Find(Name))
        {
            if (Var->Type == ESFNarrativeVariableType::Float)
            {
                OutValue = Var->FloatValue;
                return true;
            }
        }
        OutValue = 0.0f;
        return false;
    }

    UFUNCTION(BlueprintPure, Category = "Narrative|Variables")
    bool GetBool(FName Name, bool& OutValue) const
    {
        if (const FSFNarrativeVariableValue* Var = Variables.Find(Name))
        {
            if (Var->Type == ESFNarrativeVariableType::Bool)
            {
                OutValue = Var->BoolValue;
                return true;
            }
        }
        OutValue = false;
        return false;
    }

    UFUNCTION(BlueprintPure, Category = "Narrative|Variables")
    bool GetName(FName Name, FName& OutValue) const
    {
        if (const FSFNarrativeVariableValue* Var = Variables.Find(Name))
        {
            if (Var->Type == ESFNarrativeVariableType::Name)
            {
                OutValue = Var->NameValue;
                return true;
            }
        }
        OutValue = NAME_None;
        return false;
    }

    UFUNCTION(BlueprintPure, Category = "Narrative|Variables")
    bool GetTag(FName Name, FGameplayTag& OutTag) const
    {
        if (const FSFNarrativeVariableValue* Var = Variables.Find(Name))
        {
            if (Var->Type == ESFNarrativeVariableType::Tag)
            {
                OutTag = Var->TagValue;
                return true;
            }
        }
        OutTag = FGameplayTag();
        return false;
    }

    UFUNCTION(BlueprintPure, Category = "Narrative|Variables")
    bool GetTagContainer(FName Name, FGameplayTagContainer& OutTags) const
    {
        OutTags.Reset();
        if (const FSFNarrativeVariableValue* Var = Variables.Find(Name))
        {
            if (Var->Type == ESFNarrativeVariableType::TagContainer)
            {
                OutTags = Var->TagContainerValue;
                return true;
            }
        }
        return false;
    }

    //
    // Arithmetic helpers
    //

    UFUNCTION(BlueprintCallable, Category = "Narrative|Variables")
    void AddInt(FName Name, int32 Delta)
    {
        if (Name.IsNone()) return;

        FSFNarrativeVariableValue& Var = Variables.FindOrAdd(Name);
        if (Var.Type == ESFNarrativeVariableType::Int)
        {
            Var.IntValue += Delta;
        }
        else if (Var.Type == ESFNarrativeVariableType::None)
        {
            Var.Reset();
            Var.Type = ESFNarrativeVariableType::Int;
            Var.IntValue = Delta;
        }
    }

    UFUNCTION(BlueprintCallable, Category = "Narrative|Variables")
    void AddFloat(FName Name, float Delta)
    {
        if (Name.IsNone()) return;

        FSFNarrativeVariableValue& Var = Variables.FindOrAdd(Name);
        if (Var.Type == ESFNarrativeVariableType::Float)
        {
            Var.FloatValue += Delta;
        }
        else if (Var.Type == ESFNarrativeVariableType::None)
        {
            Var.Reset();
            Var.Type = ESFNarrativeVariableType::Float;
            Var.FloatValue = Delta;
        }
    }

    //
    // Serialization helpers: mirror to/from world?fact style snapshots
    //

    /** Export all variables into world?fact snapshots using a shared base tag/context. */
    void BuildSnapshots(
        TArray<FSFWorldFactSnapshot>& OutSnapshots,
        const FGameplayTag& FactTagBase,
        FName ContextIdBase = NAME_None) const
    {
        OutSnapshots.Reset();
        OutSnapshots.Reserve(Variables.Num());

        for (const TPair<FName, FSFNarrativeVariableValue>& Pair : Variables)
        {
            const FName VarName = Pair.Key;
            const FSFNarrativeVariableValue& Var = Pair.Value;

            FSFWorldFactSnapshot Snapshot;
            Snapshot.Key.FactTag = FactTagBase;
            Snapshot.Key.ContextId = ContextIdBase;
            Snapshot.Key.KeyOverride = VarName;

            switch (Var.Type)
            {
            case ESFNarrativeVariableType::Int:
                Snapshot.Value.Type = ESFNarrativeFactValueType::Int;
                Snapshot.Value.IntValue = Var.IntValue;
                break;

            case ESFNarrativeVariableType::Float:
                Snapshot.Value.Type = ESFNarrativeFactValueType::Float;
                Snapshot.Value.FloatValue = Var.FloatValue;
                break;

            case ESFNarrativeVariableType::Bool:
                Snapshot.Value.Type = ESFNarrativeFactValueType::Bool;
                Snapshot.Value.BoolValue = Var.BoolValue;
                break;

            case ESFNarrativeVariableType::Name:
                Snapshot.Value.Type = ESFNarrativeFactValueType::Name;
                Snapshot.Value.NameValue = Var.NameValue;
                break;

            case ESFNarrativeVariableType::Tag:
                Snapshot.Value.Type = ESFNarrativeFactValueType::Tag;
                Snapshot.Value.TagContainer.Reset();
                Snapshot.Value.TagContainer.AddTag(Var.TagValue);
                break;

            case ESFNarrativeVariableType::TagContainer:
                Snapshot.Value.Type = ESFNarrativeFactValueType::Tag;
                Snapshot.Value.TagContainer = Var.TagContainerValue;
                break;

            default:
                continue; // Skip None.
            }

            OutSnapshots.Add(MoveTemp(Snapshot));
        }
    }

    /** Import variables from world?fact snapshots with a shared base tag/context. */
    void RestoreFromSnapshots(
        const TArray<FSFWorldFactSnapshot>& Snapshots,
        const FGameplayTag& FactTagBase,
        FName ContextIdBase = NAME_None)
    {
        Reset();

        for (const FSFWorldFactSnapshot& Snapshot : Snapshots)
        {
            if (Snapshot.Key.FactTag != FactTagBase)
            {
                continue;
            }

            if (!ContextIdBase.IsNone() && Snapshot.Key.ContextId != ContextIdBase)
            {
                continue;
            }

            const FName VarName = Snapshot.Key.KeyOverride.IsNone()
                ? Snapshot.Key.ContextId
                : Snapshot.Key.KeyOverride;

            if (VarName.IsNone())
            {
                continue;
            }

            FSFNarrativeVariableValue Var;

            switch (Snapshot.Value.Type)
            {
            case ESFNarrativeFactValueType::Int:
                Var.Type = ESFNarrativeVariableType::Int;
                Var.IntValue = Snapshot.Value.IntValue;
                break;

            case ESFNarrativeFactValueType::Float:
                Var.Type = ESFNarrativeVariableType::Float;
                Var.FloatValue = Snapshot.Value.FloatValue;
                break;

            case ESFNarrativeFactValueType::Bool:
                Var.Type = ESFNarrativeVariableType::Bool;
                Var.BoolValue = Snapshot.Value.BoolValue;
                break;

            case ESFNarrativeFactValueType::Name:
                Var.Type = ESFNarrativeVariableType::Name;
                Var.NameValue = Snapshot.Value.NameValue;
                break;

            case ESFNarrativeFactValueType::Tag:
                if (Snapshot.Value.TagContainer.Num() == 1)
                {
                    Var.Type = ESFNarrativeVariableType::Tag;
                    Var.TagValue = *Snapshot.Value.TagContainer.CreateConstIterator();
                }
                else
                {
                    Var.Type = ESFNarrativeVariableType::TagContainer;
                    Var.TagContainerValue = Snapshot.Value.TagContainer;
                }
                break;

            default:
                continue;
            }

            Variables.Add(VarName, Var);
        }
    }
};