using UnrealBuildTool;

public class MonsterTruckTerrainTarget : TargetRules
{
	public MonsterTruckTerrainTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MonsterTruckTerrain");
	}
}
