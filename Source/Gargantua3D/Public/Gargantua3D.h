#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * The game module.
 *
 * On startup it runs the rules self-check, so "the engine-free rules work
 * inside Unreal" is something you read in the log rather than something
 * assumed because the module linked.
 */
class FGargantua3DModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override;
};
