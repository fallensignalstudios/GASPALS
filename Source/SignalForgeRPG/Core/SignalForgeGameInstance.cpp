#include "Core/SignalForgeGameInstance.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Core/SignalForgeLogChannels.h"

void USignalForgeGameInstance::Init()
{
	Super::Init();

	FSignalForgeGameplayTags::InitializeNativeGameplayTags();

	UE_LOG(LogSignalForge, Log, TEXT("Signal Forge gameplay tags initialized."));
}