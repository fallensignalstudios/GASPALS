#include "Save/SFPlayerSaveGame.h"

USFPlayerSaveGame::USFPlayerSaveGame()
{
	// Stamp the timestamp on construction so a freshly-made (un-written)
	// save still has a reasonable default if it ever leaks into a UI.
	SaveTimestamp = FDateTime::UtcNow();
}
