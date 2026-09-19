using UnrealBuildTool;

public class Gargantua3DTarget : TargetRules
{
	public Gargantua3DTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Gargantua3D");
	}
}
