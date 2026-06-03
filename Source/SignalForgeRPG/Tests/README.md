# SignalForgeRPG Functional Tests

Actor-based functional tests for in-engine scenarios that can't be covered
by pure C++ unit tests. Each test is an `AFunctionalTest` subclass placed
in a dedicated test map.

## SFRespawnLoadoutTest

Verifies that a player who dies and respawns gets their weapon's animation
state (overlay mode, upper-body overlay flag, linked anim layer class)
fully restored on the fresh pawn.

### Designer setup

1. Create a new map (e.g. `Content/Tests/L_RespawnLoadoutTest.umap`).
2. Drop a `PlayerStart`.
3. Drop a `NavMeshBoundsVolume` covering the play area -- the game-mode
   respawn flow projects onto navmesh and falls back to a lateral offset
   if absent, but tests should run on real navmesh.
4. Set the World Settings' Game Mode to your project's `BP_SFGameMode`
   so the death + respawn path matches gameplay.
5. Drop an `ASFRespawnLoadoutTest` actor.
6. On that actor, set:
   - `WeaponToEquip` -- pick any `USFWeaponData` asset that has a non-default
     animation profile (overlay mode + upper-body linked layer). Without a
     configured anim profile the test still runs but only verifies the
     definition/slot roundtrip, not the overlay binding.
   - `SlotToEquip` -- defaults to `PrimaryWeapon`; usually leave alone.
   - `PostRespawnWaitSeconds` -- defaults to 1.0s; raise if your project
     adds latency between `OnPossess` and the deferred snapshot restore.

### Running

Editor: `Tools -> Session Frontend -> Automation` tab. Filter for
`Project.Functional Tests` and run `L_RespawnLoadoutTest`.

CLI:
```
UnrealEditor-Cmd.exe SignalForgeRPG.uproject -ExecCmds="Automation RunTests Project.Functional Tests.L_RespawnLoadoutTest; Quit" -unattended -nopause -testexit="Automation Test Queue Empty"
```

### What it covers / doesn't cover

Covers:
- `SnapshotLoadoutForRespawn` captures the active slot weapon.
- `RestoreLoadoutAfterRespawn` + deferred `ApplyEquipmentSnapshotToFreshPawn`
  push the weapon back onto the new pawn.
- `EquipWeaponInstance` -> `OnWeaponEquipped` -> `ApplyWeaponAnimationFromData`
  ran on the fresh pawn (because that's the only path that sets
  `CurrentOverlayLinkedAnimLayerClass`).

Doesn't cover yet (future tests):
- Inventory contents roundtrip (`SetInventoryEntriesFromSave`).
- Multi-slot snapshot (currently only the active slot is verified;
  add slot-by-slot assertions if regressions show up there).
- Ability bar / HUD widget controller rebinding.
- Checkpoint-based vs in-place respawn divergence.
