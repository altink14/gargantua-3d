using UnrealBuildTool;

public class Gargantua3DEditorTarget : TargetRules
{
	public Gargantua3DEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Gargantua3D");
	}
}
