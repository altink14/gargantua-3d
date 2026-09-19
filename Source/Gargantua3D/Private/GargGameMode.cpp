#include "GargGameMode.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GargCavern.h"
#include "GargSlinger.h"
#include "GargantuaRules.h"

AGargGameMode::AGargGameMode()
{
	DefaultPawnClass = AGargSlinger::StaticClass();
}

void AGargGameMode::StartPlay()
{
	Super::StartPlay();

	AGargCavern* Cavern = nullptr;
	for (TActorIterator<AGargCavern> It(GetWorld()); It; ++It)
	{
		Cavern = *It;
		break;
	}
	if (Cavern == nullptr)
	{
		UE_LOG(LogGargantua, Warning,
			TEXT("No cavern in this level, so the slinger stays where it spawned."));
		return;
	}

	APlayerController* Player = GetWorld()->GetFirstPlayerController();
	APawn* Pawn = Player != nullptr ? Player->GetPawn() : nullptr;
	if (Pawn != nullptr)
	{
		const FVector Arrival = Cavern->GetArrivalLocation();
		Pawn->SetActorLocation(Arrival, false, nullptr, ETeleportType::TeleportPhysics);
		UE_LOG(LogGargantua, Display, TEXT("Slinger placed at the cavern's arrival point %s."),
			*Arrival.ToCompactString());
	}
}
