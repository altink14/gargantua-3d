using System.IO;
using UnrealBuildTool;

public class Gargantua3D : ModuleRules
{
	public Gargantua3D(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
		});

		// The rules live outside this module on purpose.
		//
		// Core/ is plain C++ with no Unreal in it at all, which is what lets its
		// tests and the balance harness run without an editor — that separation
		// is the reason the design could be proven before any of it was drawn.
		// Unreal only ever reads those headers; it never owns them.
		string CoreDir = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "Core", "src"));
		PublicIncludePaths.Add(CoreDir);

		// Core is standard C++20. Unreal 5.8 already compiles at C++20, so this
		// is stated rather than changed.
		CppStandard = CppStandardVersion.Cpp20;

		// Exceptions are deliberately NOT enabled. Core never throws — an
		// illegal action returns an explanation rather than raising — so the
		// only thing switching them on achieved was forcing an unusual shared
		// PCH variant that the engine's own headers are not warning-clean in.
	}
}
