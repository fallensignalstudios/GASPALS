#include "SFQuestDefinition.h"

FPrimaryAssetId USFQuestDefinition::GetPrimaryAssetId() const
{
    const FName EffectiveId = !QuestId.IsNone() ? QuestId : GetFName();
    return FPrimaryAssetId(TEXT("Quest"), EffectiveId);
}