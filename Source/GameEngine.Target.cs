using UnrealBuildTool;
using System.Collections.Generic;

/** Build target for the packaged game. */
public class GameEngineTarget : TargetRules
{
	public GameEngineTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("GameEngine");
	}
}
