// Copyright Fallen Signal Studios LLC. All Rights Reserved.

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

    void Reset()
    {
        Variables.Reset();
    }

    void ClearVariable(FName Name)
    {
        Variables.Remove(Name);
    }

    bool HasVariable(FName Name) const
    {
        return Variables.Contains(Name);
    }

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

    void SetInt(FName Name, int32 Value)
    {
        if (Name.IsNone()) return;
        FSFNarrativeVariableValue& Var = Variables.FindOrAdd(Name);
        Var.Reset();
        Var.Type = ESFNarrativeVariableType::Int;
        Var.IntValue = Value;
    }

    void SetFloat(FName Name, float Value)
    {
        if (Name.IsNone()) return;
        FSFNarrativeVariableValue& Var = Variables.FindOrAdd(Name);
        Var.Reset();
        Var.Type = ESFNarrativeVariableType::Float;
        Var.FloatValue = Value;
    }

    void SetBool(FName Name, bool bValue)
    {
        if (Name.IsNone()) return;
        FSFNarrativeVariableValue& Var = Variables.FindOrAdd(Name);
        Var.Reset();
        Var.Type = ESFNarrativeVariableType::Bool;
        Var.BoolValue = bValue;
    }

    void SetName(FName Name, FName Value)
    {
        if (Name.IsNone()) return;
        FSFNarrativeVariableValue& Var = Variables.FindOrAdd(Name);
        Var.Reset();
        Var.Type = ESFNarrativeVariableType::Name;
        Var.NameValue = Value;
    }

    void SetTag(FName Name, FGameplayTag Tag)
    {
        if (Name.IsNone()) return;
        FSFNarrativeVariableValue& Var = Variables.FindOrAdd(Name);
        Var.Reset();
        Var.Type = ESFNarrativeVariableType::Tag;
        Var.TagValue = Tag;
    }

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
    // Serialization helpers: mirror to/from world-fact style snapshots
    //

    /** Export all variables into world-fact snapshots using a shared base tag/context. */
    void BuildSnapshots(
        TArray<FSFWorldFactSnapshot>& OutSnapshots,
        const FGameplayTag& FactTagBase,
        FName ContextIdBase = NAME_None) const
    {
        auto MakeVariableContext = [ContextIdBase](FName VarName)
        {
            if (VarName.IsNone())
            {
                return ContextIdBase;
            }

            if (ContextIdBase.IsNone())
            {
                return VarName;
            }

            return FName(*FString::Printf(TEXT("%s|%s"), *ContextIdBase.ToString(), *VarName.ToString()));
        };

        OutSnapshots.Reset();
        OutSnapshots.Reserve(Variables.Num());

        for (const TPair<FName, FSFNarrativeVariableValue>& Pair : Variables)
        {
            const FName VarName = Pair.Key;
            const FSFNarrativeVariableValue& Var = Pair.Value;
            const FName EncodedContextId = MakeVariableContext(VarName);

            auto AddSnapshot = [&](const FSFWorldFactValue& Value)
            {
                FSFWorldFactSnapshot Snapshot;
                Snapshot.Key.FactTag = FactTagBase;
                Snapshot.Key.ContextId = EncodedContextId;
                Snapshot.Value = Value;
                OutSnapshots.Add(MoveTemp(Snapshot));
            };

            switch (Var.Type)
            {
            case ESFNarrativeVariableType::Int:
                AddSnapshot(FSFWorldFactValue::MakeInt(Var.IntValue));
                break;

            case ESFNarrativeVariableType::Float:
                AddSnapshot(FSFWorldFactValue::MakeFloat(Var.FloatValue));
                break;

            case ESFNarrativeVariableType::Bool:
                AddSnapshot(FSFWorldFactValue::MakeBool(Var.BoolValue));
                break;

            case ESFNarrativeVariableType::Name:
                AddSnapshot(FSFWorldFactValue::MakeName(Var.NameValue));
                break;

            case ESFNarrativeVariableType::Tag:
                if (Var.TagValue.IsValid())
                {
                    AddSnapshot(FSFWorldFactValue::MakeTag(Var.TagValue));
                }
                break;

            case ESFNarrativeVariableType::TagContainer:
                // TODO(narrative): tag containers flatten to one snapshot per tag; revisit if true multi-tag values are needed.
                for (const FGameplayTag Tag : Var.TagContainerValue)
                {
                    if (Tag.IsValid())
                    {
                        AddSnapshot(FSFWorldFactValue::MakeTag(Tag));
                    }
                }
                break;

            default:
                break;
            }
        }
    }

    /** Import variables from world-fact snapshots with a shared base tag/context. */
    void RestoreFromSnapshots(
        const TArray<FSFWorldFactSnapshot>& Snapshots,
        const FGameplayTag& FactTagBase,
        FName ContextIdBase = NAME_None)
    {
        auto ExtractVariableName = [ContextIdBase](FName EncodedContextId, FName& OutVarName)
        {
            OutVarName = NAME_None;

            if (EncodedContextId.IsNone())
            {
                return false;
            }

            const FString Encoded = EncodedContextId.ToString();
            if (ContextIdBase.IsNone())
            {
                OutVarName = FName(*Encoded);
                return !OutVarName.IsNone();
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

            OutVarName = FName(*Suffix);
            return !OutVarName.IsNone();
        };

        Reset();

        for (const FSFWorldFactSnapshot& Snapshot : Snapshots)
        {
            if (Snapshot.Key.FactTag != FactTagBase)
            {
                continue;
            }

            FName VarName = NAME_None;
            if (!ExtractVariableName(Snapshot.Key.ContextId, VarName))
            {
                continue;
            }

            FSFNarrativeVariableValue* ExistingVar = Variables.Find(VarName);
            FSFNarrativeVariableValue Var = ExistingVar ? *ExistingVar : FSFNarrativeVariableValue{};

            switch (Snapshot.Value.ValueType)
            {
            case ESFNarrativeFactValueType::Int:
                Var.Reset();
                Var.Type = ESFNarrativeVariableType::Int;
                Var.IntValue = Snapshot.Value.IntValue;
                break;

            case ESFNarrativeFactValueType::Float:
                Var.Reset();
                Var.Type = ESFNarrativeVariableType::Float;
                Var.FloatValue = Snapshot.Value.FloatValue;
                break;

            case ESFNarrativeFactValueType::Bool:
                Var.Reset();
                Var.Type = ESFNarrativeVariableType::Bool;
                Var.BoolValue = Snapshot.Value.BoolValue;
                break;

            case ESFNarrativeFactValueType::Name:
                Var.Reset();
                Var.Type = ESFNarrativeVariableType::Name;
                Var.NameValue = Snapshot.Value.NameValue;
                break;

            case ESFNarrativeFactValueType::Tag:
                if (!Snapshot.Value.TagValue.IsValid())
                {
                    continue;
                }

                if (Var.Type == ESFNarrativeVariableType::TagContainer)
                {
                    Var.TagContainerValue.AddTag(Snapshot.Value.TagValue);
                }
                else if (Var.Type == ESFNarrativeVariableType::Tag && Var.TagValue.IsValid() && Var.TagValue != Snapshot.Value.TagValue)
                {
                    const FGameplayTag ExistingTag = Var.TagValue;
                    Var.Reset();
                    Var.Type = ESFNarrativeVariableType::TagContainer;
                    Var.TagContainerValue.AddTag(ExistingTag);
                    Var.TagContainerValue.AddTag(Snapshot.Value.TagValue);
                }
                else
                {
                    Var.Reset();
                    Var.Type = ESFNarrativeVariableType::Tag;
                    Var.TagValue = Snapshot.Value.TagValue;
                }
                break;

            default:
                continue;
            }

            Variables.Add(VarName, Var);
        }
    }
};
