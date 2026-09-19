#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "GargGameMode.generated.h"

/**
 * Puts a slinger in a cavern.
 *
 * If the level contains a cavern actor, the player is moved to that cavern's
 * arrival point rather than to a PlayerStart. The arrival points are part of
 * the layout data and are verified walkable by the content report, so trusting
 * them is safer than trusting a marker somebody dragged into a level.
 */
UCLASS()
class GARGANTUA3D_API AGargGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGargGameMode();

protected:
	virtual void StartPlay() override;
};
