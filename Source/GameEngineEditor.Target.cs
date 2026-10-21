using UnrealBuildTool;
using System.Collections.Generic;

/** Build target for the Unreal Editor with this project's code loaded. */
public class GameEngineEditorTarget : TargetRules
{
	public GameEngineEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("GameEngine");
	}
}
