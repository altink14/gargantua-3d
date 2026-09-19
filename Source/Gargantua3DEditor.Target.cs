using UnrealBuildTool;

public class Gargantua3DEditorTarget : TargetRules
{
	public Gargantua3DEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		// V7 matches the installed engine's defaults. Anything older changes
		// warning levels relative to UnrealEditor, which is refused outright
		// for a target that shares build products with it.
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Gargantua3D");
	}
}
