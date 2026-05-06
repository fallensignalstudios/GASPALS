// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeSaveTypes.h"
#include "SFNarrativeNetDeltaTypes.generated.h"

/**
 * Per-axis identity value suitable for direct net-delta replication.
 */
USTRUCT()
struct FSFIdentityAxisNetValue
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag AxisTag;

    UPROPERTY()
    float Value = 0.0f;

    bool operator==(const FSFIdentityAxisNetValue& Other) const
    {
        return AxisTag == Other.AxisTag && FMath::IsNearlyEqual(Value, Other.Value);
    }
};

/**
 * FastArray item for identity axis values.
 */
USTRUCT()
struct FSFIdentityAxisNetItem : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY()
    FSFIdentityAxisNetValue Data;
};

/**
 * FastArray for identity axis values. This is useful if you want to
 * replicate the *current axes* wholesale and then apply narrative
 * deltas on top, or if you want clients to join mid-session and get
 * a compact axis state without a full save-game snapshot.
 */
USTRUCT()
struct FSFIdentityAxisNetArray : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FSFIdentityAxisNetItem> Items;

    UPROPERTY(NotReplicated)
    UObject* Owner = nullptr;

    FSFIdentityAxisNetArray() : Owner(nullptr) {}
    FSFIdentityAxisNetArray(UObject* InOwner) : Owner(InOwner) {}

    void SetAxisValue(FGameplayTag AxisTag, float NewValue)
    {
        for (FSFIdentityAxisNetItem& Item : Items)
        {
            if (Item.Data.AxisTag == AxisTag)
            {
                Item.Data.Value = NewValue;
                MarkItemDirty(Item);
                return;
            }
        }

        FSFIdentityAxisNetItem& NewItem = Items.Emplace_GetRef();
        NewItem.Data.AxisTag = AxisTag;
        NewItem.Data.Value = NewValue;
        MarkItemDirty(NewItem);
    }

    void RemoveAxis(FGameplayTag AxisTag)
    {
        for (int32 Index = 0; Index < Items.Num(); ++Index)
        {
            if (Items[Index].Data.AxisTag == AxisTag)
            {
                MarkItemDirty(Items[Index]);
                Items.RemoveAt(Index);
                MarkArrayDirty();
                break;
            }
        }
    }

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FSFIdentityAxisNetItem, FSFIdentityAxisNetArray>(Items, DeltaParms, *this);
    }
};

template<>
struct TStructOpsTypeTraits<FSFIdentityAxisNetArray> : public TStructOpsTypeTraitsBase2<FSFIdentityAxisNetArray>
{
    enum { WithNetDeltaSerializer = true };
};


/**
 * Per-faction standing value suitable for direct net-delta replication.
 */
USTRUCT()
struct FSFFactionStandingNetValue
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag FactionTag;

    UPROPERTY()
    float AggregateScore = 0.0f;

    UPROPERTY()
    ESFFactionStandingBand Band = ESFFactionStandingBand::Unknown;

    bool operator==(const FSFFactionStandingNetValue& Other) const
    {
        return FactionTag == Other.FactionTag
            && Band == Other.Band
            && FMath::IsNearlyEqual(AggregateScore, Other.AggregateScore);
    }
};

/**
 * FastArray item for faction standings.
 */
USTRUCT()
struct FSFFactionStandingNetItem : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY()
    FSFFactionStandingNetValue Data;
};

/**
 * FastArray wrapper for faction standings.
 */
USTRUCT()
struct FSFFactionStandingNetArray : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FSFFactionStandingNetItem> Items;

    UPROPERTY(NotReplicated)
    UObject* Owner = nullptr;

    FSFFactionStandingNetArray() : Owner(nullptr) {}
    FSFFactionStandingNetArray(UObject* InOwner) : Owner(InOwner) {}

    void SetFactionStanding(const FSFFactionStandingNetValue& NetValue)
    {
        for (FSFFactionStandingNetItem& Item : Items)
        {
            if (Item.Data.FactionTag == NetValue.FactionTag)
            {
                Item.Data = NetValue;
                MarkItemDirty(Item);
                return;
            }
        }

        FSFFactionStandingNetItem& NewItem = Items.Emplace_GetRef();
        NewItem.Data = NetValue;
        MarkItemDirty(NewItem);
    }

    void RemoveFaction(FGameplayTag FactionTag)
    {
        for (int32 Index = 0; Index < Items.Num(); ++Index)
        {
            if (Items[Index].Data.FactionTag == FactionTag)
            {
                MarkItemDirty(Items[Index]);
                Items.RemoveAt(Index);
                MarkArrayDirty();
                break;
            }
        }
    }

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FSFFactionStandingNetItem, FSFFactionStandingNetArray>(Items, DeltaParms, *this);
    }
};

template<>
struct TStructOpsTypeTraits<FSFFactionStandingNetArray> : public TStructOpsTypeTraitsBase2<FSFFactionStandingNetArray>
{
    enum { WithNetDeltaSerializer = true };
};


/**
 * Compact representation of world-fact changes for NetDelta.
 * Typically you won't stream *all* facts, only selected ones
 * (e.g. those marked "ReplicateToClients").
 */
USTRUCT()
struct FSFWorldFactNetValue
{
    GENERATED_BODY()

    UPROPERTY()
    FSFWorldFactKey Key;

    UPROPERTY()
    FSFWorldFactValue Value;
};

USTRUCT()
struct FSFWorldFactNetItem : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY()
    FSFWorldFactNetValue Data;
};

USTRUCT()
struct FSFWorldFactNetArray : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FSFWorldFactNetItem> Items;

    UPROPERTY(NotReplicated)
    UObject* Owner = nullptr;

    FSFWorldFactNetArray() : Owner(nullptr) {}
    FSFWorldFactNetArray(UObject* InOwner) : Owner(InOwner) {}

    void SetFact(const FSFWorldFactNetValue& NetValue)
    {
        for (FSFWorldFactNetItem& Item : Items)
        {
            if (Item.Data.Key == NetValue.Key)
            {
                Item.Data.Value = NetValue.Value;
                MarkItemDirty(Item);
                return;
            }
        }

        FSFWorldFactNetItem& NewItem = Items.Emplace_GetRef();
        NewItem.Data = NetValue;
        MarkItemDirty(NewItem);
    }

    void RemoveFact(const FSFWorldFactKey& Key)
    {
        for (int32 Index = 0; Index < Items.Num(); ++Index)
        {
            if (Items[Index].Data.Key == Key)
            {
                MarkItemDirty(Items[Index]);
                Items.RemoveAt(Index);
                MarkArrayDirty();
                break;
            }
        }
    }

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FSFWorldFactNetItem, FSFWorldFactNetArray>(Items, DeltaParms, *this);
    }
};

template<>
struct TStructOpsTypeTraits<FSFWorldFactNetArray> : public TStructOpsTypeTraitsBase2<FSFWorldFactNetArray>
{
    enum { WithNetDeltaSerializer = true };
};
