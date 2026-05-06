// Copyright Epic Games, Inc. All Rights Reserved.

#include "SignalForgeRPG.h"
#include "Modules/ModuleManager.h"
#include "Core/SignalForgeGameplayTags.h"

class FSignalForgeRPGModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		FSignalForgeGameplayTags::InitializeNativeGameplayTags();
	}

	virtual void ShutdownModule() override
	{
		FDefaultGameModuleImpl::ShutdownModule();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FSignalForgeRPGModule, SignalForgeRPG, "SignalForgeRPG");