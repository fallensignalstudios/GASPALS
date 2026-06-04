// SignalForgeLogChannels.h
//
// Central declaration of public LogSF* categories. Prefer these over LogTemp
// and over per-file DEFINE_LOG_CATEGORY_STATIC -- a single category per
// subsystem keeps QA's runtime toggling sane (Session Frontend / OutputLog
// filter / DefaultEngine.ini [Core.Log] all key off the category name).
//
// Severity convention:
//   - Error/Fatal: build/data is broken; cannot continue meaningfully.
//   - Warning: designer mis-wired a thing that the runtime had to compensate
//              for. Should be actionable.
//   - Display: one-shot lifecycle events worth seeing without enabling Verbose.
//   - Log:     normal operational detail; default visible in editor.
//   - Verbose: per-frame / per-actor-spawn / per-tick detail. Off by default.
//
// EXCEPTION: LogSFNarrative is defined in Narrative/SFNarrativeLog.h (legacy
// home, already public via SIGNALFORGERPG_API). New code in the Narrative
// subsystem should still use it; include Narrative/SFNarrativeLog.h directly.

#pragma once

#include "CoreMinimal.h"

/** Catch-all for cross-cutting messages that don't belong to a specific subsystem. */
DECLARE_LOG_CATEGORY_EXTERN(LogSignalForge, Log, All);

/** Combat: damage pipeline, weapon abilities (melee/ranged/cast/beam/fire), block, projectiles, auto-aim. */
DECLARE_LOG_CATEGORY_EXTERN(LogSFCombat, Log, All);

/** Equipment: SFEquipmentComponent, weapon attach/spawn, slot bookkeeping, overlay state. */
DECLARE_LOG_CATEGORY_EXTERN(LogSFEquipment, Log, All);

/** Inventory: SFInventoryComponent state, save/load entry round-trip, broadcasts. */
DECLARE_LOG_CATEGORY_EXTERN(LogSFInventory, Log, All);

/** Character: SFCharacterBase, SFPlayerCharacter, anim overlays, possession, respawn. */
DECLARE_LOG_CATEGORY_EXTERN(LogSFCharacter, Log, All);

/** AI: controllers, behavior tree tasks/services/decorators, perception, patrol. */
DECLARE_LOG_CATEGORY_EXTERN(LogSFAI, Log, All);

/** Save: SFPlayerSaveService capture/restore, snapshots, persistence. */
DECLARE_LOG_CATEGORY_EXTERN(LogSFSave, Log, All);

/** UI: all widgets + widget controllers (HUD, menus, dialogue panel, quest log, prompts). */
DECLARE_LOG_CATEGORY_EXTERN(LogSFUI, Log, All);

/** Input: SFPlayerController input mapping, action dispatch, enhanced input setup. */
DECLARE_LOG_CATEGORY_EXTERN(LogSFInput, Log, All);

/** Player class system: SFPlayerClassComponent, class transitions, class-driven abilities. */
DECLARE_LOG_CATEGORY_EXTERN(LogSFPlayerClass, Log, All);
