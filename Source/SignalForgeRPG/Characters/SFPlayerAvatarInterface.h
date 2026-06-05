// SFPlayerAvatarInterface.h
//
// ISFPlayerAvatar: contract for an actor that represents "the player's
// physical presence in the world" -- the thing the dialogue camera attaches
// to, the thing the HUD reads a portrait from, the thing the weapon-firing
// abilities ask to kick the view back on recoil.
//
// This is the slice-3 interface in the dual-protagonist refactor. Both
// protagonists (current ASFPlayerCharacter, and the second protagonist when
// it lands) will implement this. The interface lets dialogue, narrative,
// UI, and feedback systems treat either protagonist uniformly without
// casting to a concrete subclass.
//
// Design notes:
//   - Surface is driven by actual consumer call sites, not speculation.
//     Every method on this interface corresponds to a Cast<ASFPlayerCharacter>
//     site that exists in the codebase today (dialogue camera component,
//     interaction component, dialogue panel widget, HUD widget controller,
//     weapon-fire/beam abilities for recoil).
//   - "Is this a player avatar?" checks (NarrativeTriggerSphere, Checkpoint)
//     don't need a dedicated method -- Implements<USFPlayerAvatarInterface>()
//     answers it.
//   - Inventory/equipment/ammo/progression/faction live on the character
//     base layer, not the player-avatar layer. ISFWeaponHolder (slice 2)
//     already covers equipment + ammo for any holder type; the remaining
//     character-level component getters stay on ASFCharacterBase.
//   - Save service intentionally NOT routed through this interface. The
//     existing SaveService takes ASFCharacterBase* and operates on
//     character-level components (Equipment, Ammo, Inventory, Progression);
//     none of those are player-avatar-specific.

#pragma once

#include "UObject/Interface.h"
#include "SFPlayerAvatarInterface.generated.h"

class USFDialogueComponent;
class USceneComponent;
class UCameraComponent;
class UTextureRenderTarget2D;

UINTERFACE(MinimalAPI, Blueprintable)
class USFPlayerAvatarInterface : public UInterface
{
	GENERATED_BODY()
};

class SIGNALFORGERPG_API ISFPlayerAvatarInterface
{
	GENERATED_BODY()

public:
	/** Dialogue runtime component (line playback, choices, conversation state). */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Avatar|Dialogue")
	USFDialogueComponent* GetDialogueComponent() const;

	/** Scene-component anchor the dialogue camera framing solver uses as its root pivot. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Avatar|Dialogue")
	USceneComponent* GetDialogueCameraRoot() const;

	/** The dedicated dialogue camera (activated during conversations, deactivated otherwise). */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Avatar|Dialogue")
	UCameraComponent* GetDialogueCamera() const;

	/** The standard gameplay camera (deactivated while the dialogue camera is live, reactivated afterward). */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Avatar|Camera")
	UCameraComponent* GetGameplayCamera() const;

	/** Render target used by the HUD portrait widget. May be null on protagonists that don't render a portrait. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Avatar|HUD")
	UTextureRenderTarget2D* GetPortraitRenderTarget() const;

	/**
	 * Apply a per-shot recoil kick to the avatar's view. NPCs that implement this interface
	 * should leave the default impl (which does nothing) -- they have no player camera to nudge.
	 *
	 * PitchDegrees pushes the view UP (positive value), YawDegrees may be positive or negative.
	 * InterpSpeed controls kick-on rate; RecoverySpeed controls return rate; RecoveryFraction is
	 * how much of the kick the return ultimately recovers (0..1).
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Avatar|Feedback")
	void ApplyRecoilKick(float PitchDegrees, float YawDegrees, float InterpSpeed, float RecoverySpeed, float RecoveryFraction);
};
