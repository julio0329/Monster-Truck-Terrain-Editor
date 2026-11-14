using UnrealBuildTool;

public class MonsterTruckTerrain : ModuleRules
{
	public MonsterTruckTerrain(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"ProceduralMeshComponent"
		});
	}
}
