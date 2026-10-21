using UnrealBuildTool;

/** Module rules for the GameEngine game module. */
public class GameEngine : ModuleRules
{
	public GameEngine(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"RenderCore",
			"EnhancedInput",
			"UMG",
			"Slate",
			"SlateCore"
		});
	}
}
