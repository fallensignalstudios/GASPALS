// SignalForgeDeveloperSettings.cpp

#include "Core/SignalForgeDeveloperSettings.h"

#include "Faction/SFFactionRelationshipData.h"

USignalForgeDeveloperSettings::USignalForgeDeveloperSettings()
{
}

const USFFactionRelationshipData* USignalForgeDeveloperSettings::GetDefaultFactionRelationships() const
{
	if (DefaultFactionRelationships.IsNull())
	{
		return nullptr;
	}

	// Synchronous load is acceptable at gameplay query time: this asset is small,
	// and faction queries happen far less often than per-frame logic. CDOs will
	// hit the cached weak ptr on subsequent calls.
	return DefaultFactionRelationships.LoadSynchronous();
}