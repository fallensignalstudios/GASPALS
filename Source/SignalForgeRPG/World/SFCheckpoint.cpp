#include "World/SFCheckpoint.h"

#include "Characters/SFPlayerCharacter.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/SFGameStateBase.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

ASFCheckpoint::ASFCheckpoint()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("TriggerCapsule"));
	TriggerCapsule->InitCapsuleSize(140.0f, 140.0f);
	TriggerCapsule->SetCollisionProfileName(TEXT("Trigger"));
	TriggerCapsule->SetGenerateOverlapEvents(true);
	RootComponent = TriggerCapsule;

#if WITH_EDITORONLY_DATA
	Billboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	if (Billboard)
	{
		Billboard->SetupAttachment(RootComponent);
	}

	SpawnDirection = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("SpawnDirection"));
	if (SpawnDirection)
	{
		SpawnDirection->SetupAttachment(RootComponent);
		SpawnDirection->ArrowColor = FColor(80, 200, 255);
		SpawnDirection->ArrowSize = 1.5f;
	}
#endif
}

void ASFCheckpoint::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerCapsule && bActivateOnPlayerOverlap)
	{
		TriggerCapsule->OnComponentBeginOverlap.AddDynamic(this, &ASFCheckpoint::HandleOverlapBegin);
	}
}

void ASFCheckpoint::HandleOverlapBegin(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	if (ASFPlayerCharacter* Player = Cast<ASFPlayerCharacter>(OtherActor))
	{
		ActivateCheckpoint(Player);
	}
}

void ASFCheckpoint::ActivateCheckpoint(ASFPlayerCharacter* ActivatingPlayer)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (ASFGameStateBase* GS = World->GetGameState<ASFGameStateBase>())
	{
		// Game state owns the "last touched checkpoint" so it survives
		// pawn destruction during respawn and is reachable from both the
		// player controller (to query) and game mode (to respawn against).
		GS->SetActiveCheckpoint(this, ActivatingPlayer);
	}
}

FTransform ASFCheckpoint::GetRespawnTransform() const
{
	// Lift slightly above the trigger origin so the pawn doesn't spawn
	// half-buried if the designer placed the checkpoint on the floor.
	FTransform Out = GetActorTransform();
	Out.AddToTranslation(FVector(0.0f, 0.0f, 20.0f));
	return Out;
}
