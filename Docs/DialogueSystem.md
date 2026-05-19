# Sovereign Call — Dialogue System Guide

This is the practical, code-first reference for the runtime dialogue system in
`SignalForgeRPG/Dialogue/`. It covers the architecture, how to author
conversations, how to drive them from gameplay, and — most commonly asked —
how to respond to tagged event nodes from Blueprint or C++.

---

## 1. Architecture at a glance

```
USFConversationDataAsset (asset)
        │
        ▼
USFDialogueComponent (on the player)   ──►  USFDialogueWidgetController
        │                                            │
        │ delegates                                  │ delegates
        ▼                                            ▼
   Gameplay listeners                         Dialogue UI widgets
   (quests, AI, cinematics, etc.)             (line, choice buttons, history)
```

- **`USFConversationDataAsset`** — the authored conversation. A list of
  `FSFDialogueNode` entries plus an `EntryNodeId`. Edited via the dialogue
  graph editor; compiled into this flat node list.
- **`USFDialogueComponent`** — runtime state machine. Lives on the player
  pawn. Handles auto-advance timing, tag-gated nodes/choices, history,
  pause/resume, and snapshot save/restore.
- **`USFDialogueWidgetController`** — thin BP-facing adapter. Subscribes
  to the component, broadcasts UI-friendly events. Owned by your HUD.
- **`USFDialogueChoiceButtonWidget`** — native parent for the choice button
  widget. Enforces the layout that prevents per-word wrapping.

---

## 2. Node types

| Type | Purpose | Required fields |
|---|---|---|
| `Line` | NPC says something. Auto-advance modes available. | `LineText`, `NextNodeId` |
| `Choice` | Player picks one of `Choices[]`. | `Choices[]` (each with `ChoiceText`, `NextNodeId`) |
| `Event` | Fires a gameplay tag to listeners. Then continues to `NextNodeId`. | `EventTag`, `NextNodeId` |
| `End` | Terminates the conversation. | — |

Every node + choice supports `RequiredTags` / `BlockedTags` gating. Filtering
happens against the tag set returned by
`USFDialogueComponent::GetOwnedDialogueTags(...)` (override this in a
subclass once the ASC / quest state / faction systems are wired up).

`Line` nodes additionally support `AdvanceMode`:

- `Manual` — UI calls `AdvanceConversation()`.
- `FixedDuration` — auto-advance after `AdvanceDelaySeconds`.
- `WordsPerMinute` — auto-advance after a delay computed from
  `LineText.WordCount / WordsPerMinute`.
- `AudioFinished` — auto-advance when `VoiceClip` finishes playing.

---

## 3. Responding to tagged Event nodes

The `Event` node broadcasts its `EventTag` via:

```cpp
// USFDialogueComponent.h
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFOnDialogueEventTriggered, FGameplayTag, EventTag);

UPROPERTY(BlueprintAssignable, Category = "Dialogue")
FSFOnDialogueEventTriggered OnDialogueEventTriggered;
```

### 3a. From Blueprint

1. Get a reference to the player's `USFDialogueComponent`
   (e.g. via `SFPlayerCharacter -> GetDialogueComponent`).
2. Drag off the component, search **"Bind Event to On Dialogue Event Triggered"**.
3. Connect it to a Custom Event whose only input is `FGameplayTag EventTag`.
4. In the custom event, branch on `MatchesTag` / `MatchesTagAny` against your
   gameplay tags (e.g. `Dialogue.Event.Quest.Sergeant.AcceptMission`).

If your HUD already constructs a `USFDialogueWidgetController`, you can bind
to the equivalent **`OnDialogueEventForwarded`** delegate on the controller —
it's a forwarded copy with the same payload.

### 3b. From C++

```cpp
// MyQuestListener.h
UCLASS()
class AMyQuestListener : public AActor
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;

protected:
    UFUNCTION()
    void HandleDialogueEvent(FGameplayTag EventTag);
};
```

```cpp
// MyQuestListener.cpp
#include "Characters/SFPlayerCharacter.h"
#include "Dialogue/Data/SFDialogueComponent.h"
#include "Kismet/GameplayStatics.h"

void AMyQuestListener::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) { return; }

    ASFPlayerCharacter* Player = Cast<ASFPlayerCharacter>(PC->GetPawn());
    if (!Player) { return; }

    if (USFDialogueComponent* Dlg = Player->GetDialogueComponent())
    {
        Dlg->OnDialogueEventTriggered.AddDynamic(this, &AMyQuestListener::HandleDialogueEvent);
    }
}

void AMyQuestListener::HandleDialogueEvent(FGameplayTag EventTag)
{
    static const FGameplayTag AcceptMissionTag =
        FGameplayTag::RequestGameplayTag(TEXT("Dialogue.Event.Quest.Sergeant.AcceptMission"));

    if (EventTag.MatchesTag(AcceptMissionTag))
    {
        // ...start the quest, give the item, open a door, play a cinematic, etc.
    }
}
```

**Important:** the player pawn can be re-possessed (death, level transition,
companion swap). If your listener outlives the pawn, also unbind in
`EndPlay` and re-bind on pawn changes (`APlayerController::OnPossessedPawnChanged`).

### 3c. From a `USFUserWidgetBase` HUD subwidget

Use the widget controller's forwarded event so you don't have to chase the
component yourself:

```cpp
void UMyDialogueHistoryWidget::NativeOnPlayerHUDWidgetControllerSet()
{
    if (USFDialogueWidgetController* DC = PlayerHUDWidgetController->GetDialogueController())
    {
        DC->OnDialogueEventForwarded.AddDynamic(this, &UMyDialogueHistoryWidget::HandleDlgEvent);
    }
}
```

(You'll need to expose a `GetDialogueController()` accessor on
`USFPlayerHUDWidgetController` for that exact call — easy follow-up.)

---

## 4. Choice button widget — fixing per-word wrapping

The `WBP_DialogueChoiceButton` Blueprint should be **reparented** to
`USFDialogueChoiceButtonWidget`. The native class enforces:

- `AutoWrapText = true`
- `WrapTextAt = 0` (let layout decide where to break)
- `MinDesiredWidth = MinChoiceWidth` (default 480 px) — this is the actual
  fix for "wraps after every word". The TextBlock no longer collapses to
  its smallest word-sized desired width.
- `Justification = Left`

Bind these widgets in the Blueprint with these exact names:

| Bind name | Type | Required |
|---|---|---|
| `ChoiceButton` | `UButton` | yes |
| `PromptText` | `UTextBlock` | yes (or `PromptRichText`) |
| `PromptRichText` | `URichTextBlock` | optional |
| `PromptSizeBox` | `USizeBox` (wrapping the text block) | optional, recommended |

Call `SetChoiceText(...)` and `SetChoiceIndex(...)` from the dialogue panel,
then listen to `OnChoiceClicked(int32 ChoiceIndex)` to forward the click into
`USFDialogueWidgetController::SelectChoice(ChoiceIndex)`.

If you ever see per-word wrapping again, it's almost always one of:

1. The button is inside a `HorizontalBox` slot with `Auto` size → switch the
   slot to `Fill`, or wrap the button in a `SizeBox` with `MinDesiredWidth`.
2. The TextBlock is inside an extra `HorizontalBox` with `Auto` sizing.
3. `WrappingPolicy = AllowPerCharacterWrapping` was set in the designer —
   switch back to `DefaultWrapping`.

---

## 5. Other useful runtime APIs

### Dialogue history

```cpp
const TArray<FSFDialogueHistoryEntry>& History = Dlg->GetDialogueHistory();
for (const FSFDialogueHistoryEntry& Entry : History) { /* ... */ }
Dlg->ClearDialogueHistory();
```

`MaxHistoryEntries` on the component caps the buffer (default 256, 0
disables history entirely).

### Pause / resume

```cpp
Dlg->PauseConversation();   // freezes auto-advance + voice audio
Dlg->ResumeConversation();  // resumes with the captured remaining time
Dlg->IsConversationPaused();
```

`USFDialogueWidgetController` mirrors these as `PauseDialogue` /
`ResumeDialogue` and broadcasts `OnDialoguePauseChanged(bIsPaused)`.

### Skip line

`SkipCurrentLine()` lets the player advance immediately past a `WordsPerMinute`
or `FixedDuration` line that's still auto-advancing. No-op on choice / event /
end nodes. Returns `true` if it actually skipped.

### Snapshot save/restore

```cpp
FSFDialogueSnapshot Snap = Dlg->CreateSnapshot();
// ... persist Snap in your save game struct ...
Dlg->RestoreFromSnapshot(LoadedSnap, SourceActor);
```

The conversation asset is referenced by `TSoftObjectPtr` so it survives
unloads. `RestoreFromSnapshot` re-fires `OnConversationStarted` and re-runs
staging so the camera / positioning matches the original entry.

### Choice-selected event (for narrative branching)

```cpp
UPROPERTY(BlueprintAssignable, Category = "Dialogue")
FSFOnDialogueChoiceSelected OnDialogueChoiceSelected;
// (int32 ChoiceIndex, const FSFDialogueChoice& Choice)
```

Useful for narrative hooks that need to know *which* choice the player picked,
without having to peek at `GetCurrentDisplayData()` before the node updates.

---

## 6. Common authoring mistakes

- **`EntryNodeId` is `None`** — `ValidateConversation` will reject.
- **Duplicate `NodeId`s** — also rejected by validation.
- **Choice node with all choices gated out by tags** — runtime logs a warning
  on entry; UI will show zero buttons.
- **Event node with no `EventTag`** — fires nothing; only warning, not an
  error.
- **End node with `LineText` set** — End nodes terminate immediately; the
  text is never displayed. Author a final `Line` node before the `End`.

---

## 7. File map

| File | Purpose |
|---|---|
| `Dialogue/Data/SFDialogueTypes.h` | `FSFDialogueNode`, `FSFDialogueChoice`, `FSFDialogueDisplayData`, history + snapshot structs |
| `Dialogue/Data/SFConversationDataAsset.*` | Authored asset + validation |
| `Dialogue/Data/SFDialogueComponent.*` | Runtime state machine, history, pause, snapshot |
| `Dialogue/Data/SFDialogueWidgetController.*` | UI-facing adapter |
| `Dialogue/SFDialogueCameraComponent.*` | Camera shot application during dialogue |
| `Dialogue/DialogueGraph/*` | Editor graph nodes + compiler (editor-only path) |
| `UI/SFDialogueChoiceButtonWidget.*` | Native parent for choice buttons |
