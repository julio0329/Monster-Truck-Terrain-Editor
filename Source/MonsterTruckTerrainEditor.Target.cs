using UnrealBuildTool;

public class MonsterTruckTerrainEditorTarget : TargetRules
{
	public MonsterTruckTerrainEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MonsterTruckTerrain");
	}
}
