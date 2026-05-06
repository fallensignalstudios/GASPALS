// Copyright Fallen Signal Studios 2026.

using UnrealBuildTool;
using System.Collections.Generic;

public class SignalForgeRPGEditorTarget : TargetRules
{
	public SignalForgeRPGEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;

		ExtraModuleNames.AddRange( new string[] { "SignalForgeRPG" } );
	}
}
