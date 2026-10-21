// RayShot.Build.cs
// Lists the Unreal modules (libraries) our game code uses.

using UnrealBuildTool;

public class RayShot : ModuleRules
{
	public RayShot(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",         // basic types: FVector, FString, TArray...
			"CoreUObject",  // UCLASS / UPROPERTY system
			"Engine",       // actors, components, HUD, lights
			"InputCore"     // EKeys (mouse buttons, keyboard keys)
		});
	}
}
