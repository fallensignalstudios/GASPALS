// Copyright Fallen Signal Studios 2026.

using UnrealBuildTool;
using System.Collections.Generic;

public class SignalForgeRPGTarget : TargetRules
{
	public SignalForgeRPGTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;

		ExtraModuleNames.AddRange( new string[] { "SignalForgeRPG" } );
	}
}
