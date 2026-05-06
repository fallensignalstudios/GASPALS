#include "Core/SFPlayerState.h"
#include "Narrative/SFNarrativeComponent.h"
#include "Net/UnrealNetwork.h"

ASFPlayerState::ASFPlayerState()
{
	// PlayerState already replicates by default; bumping update frequency
	// modestly so narrative deltas reach the owning client promptly without
	// flooding the channel for non-owning observers.
	SetNetUpdateFrequency(10.0f);
	SetMinNetUpdateFrequency(2.0f);

	NarrativeComponent = CreateDefaultSubobject<USFNarrativeComponent>(TEXT("NarrativeComponent"));
	NarrativeComponent->SetIsReplicated(true);
}

void ASFPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// NarrativeComponent itself replicates as a sub-object; the component
	// declares its own replicated properties. No additional DOREPLIFETIME
	// entries are required here unless you add new UPROPERTYs to this class.
}

void ASFPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Hook for any post-construct narrative wiring (e.g. binding the
	// component to a save service on the GameInstance). Intentionally
	// minimal — most wiring happens inside the component itself.
}

UAbilitySystemComponent* ASFPlayerState::GetAbilitySystemComponent() const
{
	// Project hosts GAS on the pawn today. If you later move the
	// AbilitySystemComponent to the PlayerState, return it here.
	return nullptr;
}
