// RayShotEditor.Target.cs
// Tells Unreal how to build the project for the EDITOR.

using UnrealBuildTool;
using System.Collections.Generic;

public class RayShotEditorTarget : TargetRules
{
	public RayShotEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("RayShot");
	}
}
