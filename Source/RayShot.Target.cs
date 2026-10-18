// RayShot.Target.cs
// Tells Unreal how to build the GAME (the packaged .exe).

using UnrealBuildTool;
using System.Collections.Generic;

public class RayShotTarget : TargetRules
{
	public RayShotTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("RayShot");
	}
}
