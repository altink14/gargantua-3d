#include "Gargantua3D.h"

#include "GargantuaRules.h"

void FGargantua3DModule::StartupModule()
{
	FDefaultGameModuleImpl::StartupModule();

	// Cheap, and it fails loudly rather than silently. The alternative is
	// discovering months later that a rule changed shape in translation.
	UGargantuaRules::RunRulesSelfCheck();
}

IMPLEMENT_PRIMARY_GAME_MODULE(FGargantua3DModule, Gargantua3D, "Gargantua3D");
