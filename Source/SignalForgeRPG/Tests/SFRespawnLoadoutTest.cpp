// Copyright Fallen Signal Studios 2026.

#include "Tests/SFRespawnLoadoutTest.h"

#include "Characters/SFCharacterBase.h"
#include "Combat/SFWeaponData.h"
#include "Combat/SFWeaponInstanceTypes.h"
#include "Components/SFEquipmentComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Input/SFPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ASFRespawnLoadoutTest::ASFRespawnLoadoutTest()
{
	// AFunctionalTest defaults are mostly fine; one second is enough for the
	// scenario but designers can extend per-instance if they layer extra logic.
	TimeLimit = 15.0f;
	TimesUpMessage = NSLOCTEXT("SFRespawnLoadoutTest", "TimesUp",
		"Respawn loadout test timed out before assertions could run.");
	TimesUpResult = EFunctionalTestResult::Failed;
}

ASFCharacterBase* ASFRespawnLoadoutTest::FindPlayerCharacter() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		return nullptr;
	}

	return Cast<ASFCharacterBase>(PC->GetPawn());
}

bool ASFRespawnLoadoutTest::IsReady_Implementation()
{
	// Don't start until a player pawn exists and its equipment component is
	// constructed. Otherwise PrepareTest would race the initial possession.
	const ASFCharacterBase* Character = FindPlayerCharacter();
	return Character && Character->GetEquipmentComponent();
}

void ASFRespawnLoadoutTest::PrepareTest()
{
	Super::PrepareTest();

	// Resolve and hold a hard reference to the weapon definition so GC can't
	// reclaim it during the respawn gap.
	if (!WeaponToEquip.IsNull())
	{
		LoadedWeaponDef = WeaponToEquip.LoadSynchronous();
	}
}

void ASFRespawnLoadoutTest::StartTest()
{
	Super::StartTest();

	if (!AssertTrue(LoadedWeaponDef != nullptr,
		TEXT("WeaponToEquip must be set on the test actor before running.")))
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Missing weapon configuration."));
		return;
	}

	ASFCharacterBase* Character = FindPlayerCharacter();
	if (!AssertTrue(Character != nullptr, TEXT("No player ASFCharacterBase found at StartTest.")))
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Player character missing."));
		return;
	}

	Step_EquipAndBaseline();
}

bool ASFRespawnLoadoutTest::CaptureBaselineFromCharacter(ASFCharacterBase* Character)
{
	if (!Character)
	{
		return false;
	}

	USFEquipmentComponent* Equipment = Character->GetEquipmentComponent();
	if (!Equipment || !Equipment->GetCurrentWeaponData())
	{
		return false;
	}

	BeforeOverlayMode = static_cast<uint8>(Character->GetCurrentOverlayMode());
	bBeforeUseUpperBodyOverlay = Character->GetUseUpperBodyOverlay();
	BeforeOverlayLinkedAnimLayerClass = Character->GetCurrentOverlayLinkedAnimLayerClass();
	bBaselineCaptured = true;
	return true;
}

void ASFRespawnLoadoutTest::Step_EquipAndBaseline()
{
	ASFCharacterBase* Character = FindPlayerCharacter();
	USFEquipmentComponent* Equipment = Character ? Character->GetEquipmentComponent() : nullptr;
	if (!AssertTrue(Equipment != nullptr, TEXT("Player has no EquipmentComponent.")))
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("No equipment component."));
		return;
	}

	// Build a fresh weapon instance pointing at the configured definition.
	// We don't go through inventory here -- the test is scoped to the
	// equip + animation + respawn-restore loop, not pickup flows.
	FSFWeaponInstanceData Instance;
	Instance.InstanceId = FGuid::NewGuid();
	Instance.WeaponDefinition = WeaponToEquip;

	Equipment->EquipWeaponInstance(Instance, SlotToEquip);
	Equipment->SetActiveWeaponSlot(SlotToEquip);

	if (!AssertTrue(Equipment->GetActiveWeaponSlot() == SlotToEquip,
		TEXT("Active slot did not switch to SlotToEquip after EquipWeaponInstance.")))
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Equip did not take effect."));
		return;
	}

	if (!AssertTrue(CaptureBaselineFromCharacter(Character),
		TEXT("Could not capture overlay baseline -- weapon equip did not populate current weapon data.")))
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Baseline capture failed."));
		return;
	}

	// Give the AnimInstance one tick to settle on the equipped weapon's
	// overlay before we kill the pawn. Some animation properties update on
	// the next NativeUpdateAnimation pass after LinkAnimClassLayers lands.
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(
		Handle,
		FTimerDelegate::CreateUObject(this, &ASFRespawnLoadoutTest::Step_KillAndRequestRespawn),
		0.1f,
		false);
}

void ASFRespawnLoadoutTest::Step_KillAndRequestRespawn()
{
	ASFCharacterBase* Character = FindPlayerCharacter();
	if (!AssertTrue(Character != nullptr, TEXT("Player pawn vanished before kill step.")))
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Player missing pre-kill."));
		return;
	}

	ASFPlayerController* PC = Cast<ASFPlayerController>(Character->GetController());
	if (!AssertTrue(PC != nullptr, TEXT("Player is not possessed by ASFPlayerController.")))
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Wrong PC class."));
		return;
	}

	// Snapshot occurs inside HandlePawnDied, which is wired to OnCharacterDied.
	// HandleDeath fires that delegate, then game mode destroys the pawn and
	// RestartPlayerAtTransform spawns the new one. We skip the death-screen
	// countdown by calling RespawnFromDeathScreen directly after HandleDeath.
	Character->HandleDeath();
	PC->RespawnFromDeathScreen(false);

	// Wait for: game-mode respawn (1 frame) + OnPossess (immediate) +
	// SetTimerForNextTick for ApplyEquipmentSnapshotToFreshPawn (1 frame).
	// PostRespawnWaitSeconds is the generous wall-clock buffer over that.
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(
		Handle,
		FTimerDelegate::CreateUObject(this, &ASFRespawnLoadoutTest::Step_AssertRestored),
		FMath::Max(0.1f, PostRespawnWaitSeconds),
		false);
}

void ASFRespawnLoadoutTest::Step_AssertRestored()
{
	if (!AssertTrue(bBaselineCaptured, TEXT("Baseline was never captured -- equip step failed silently.")))
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("No baseline."));
		return;
	}

	ASFCharacterBase* Character = FindPlayerCharacter();
	if (!AssertTrue(Character != nullptr, TEXT("No player pawn after respawn -- possession never completed.")))
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Respawn did not produce a pawn."));
		return;
	}

	USFEquipmentComponent* Equipment = Character->GetEquipmentComponent();
	if (!AssertTrue(Equipment != nullptr, TEXT("New pawn has no EquipmentComponent.")))
	{
		FinishTest(EFunctionalTestResult::Failed, TEXT("Respawn pawn missing equipment."));
		return;
	}

	// 1) Weapon definition came back on the active slot.
	USFWeaponData* RestoredData = Equipment->GetCurrentWeaponData();
	AssertEqual_Object(
		RestoredData,
		LoadedWeaponDef,
		TEXT("Active weapon definition after respawn differs from pre-death definition."));

	// 2) Active slot matches what we equipped.
	AssertTrue(Equipment->GetActiveWeaponSlot() == SlotToEquip,
		TEXT("Active slot after respawn does not match the slot we equipped."));

	// 3) Overlay mode -- the scalar the AnimInstance reads via property access.
	const uint8 AfterOverlayMode = static_cast<uint8>(Character->GetCurrentOverlayMode());
	AssertEqual_Int(static_cast<int32>(AfterOverlayMode), static_cast<int32>(BeforeOverlayMode),
		TEXT("CurrentOverlayMode did not match pre-death value after respawn."));

	// 4) Upper-body overlay flag -- gates the upper-body slot in the anim graph.
	AssertEqual_Bool(Character->GetUseUpperBodyOverlay(), bBeforeUseUpperBodyOverlay,
		TEXT("bUseUpperBodyOverlay did not match pre-death value after respawn."));

	// 5) Linked anim layer class -- this is the one most likely to silently
	// regress, because it's set via LinkAnimClassLayers which no-ops if the
	// AnimInstance isn't ready. If the deferred restore lands too early this
	// will come back null while the scalars come back correct.
	AssertEqual_Object(
		Character->GetCurrentOverlayLinkedAnimLayerClass().Get(),
		BeforeOverlayLinkedAnimLayerClass.Get(),
		TEXT("Overlay linked anim layer class did not match pre-death value after respawn."));

	FinishTest(EFunctionalTestResult::Succeeded, TEXT("Respawn restored weapon overlay state."));
}
