#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GargCavern.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;

/**
 * Builds a cavern out of the layout data.
 *
 * The walls, the slugs, the slingers and the doorways are all read from Core's
 * cavern tables rather than placed by hand in the editor. That matters more
 * than it sounds: those layouts were balanced in the browser build and every
 * marker in them is verified reachable by the content report, so a blockout
 * built from the same numbers is a blockout that is already known to be
 * walkable. Moving a wall in the editor would quietly throw that away.
 *
 * This is geometry only — boxes and lights standing in for art that does not
 * exist yet. It is meant to be walked, not looked at.
 */
UCLASS()
class GARGANTUA3D_API AGargCavern : public AActor
{
	GENERATED_BODY()

public:
	AGargCavern();

	/** Which cavern to build, by its id: hollow-reach, kindled-deep, drowned-vault. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gargantua")
	FName BiomeId = TEXT("hollow-reach");

	/** How tall the walls stand, in centimetres. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gargantua")
	float WallHeight = 600.0f;

	/** Rebuilds from the layout data. Safe to call repeatedly. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Gargantua")
	void Rebuild();

	/** Where a player entering this cavern should stand, in world space. */
	UFUNCTION(BlueprintPure, Category = "Gargantua")
	FVector GetArrivalLocation() const;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Gargantua")
	TObjectPtr<USceneComponent> Root;

	/** One instanced mesh for all the rock. A cavern is mostly walls. */
	UPROPERTY(VisibleAnywhere, Category = "Gargantua")
	TObjectPtr<UInstancedStaticMeshComponent> Walls;

	/** The floor, one stretched box. */
	UPROPERTY(VisibleAnywhere, Category = "Gargantua")
	TObjectPtr<UStaticMeshComponent> Floor;

	/** Blockout stand-ins for the wild slugs. */
	UPROPERTY(VisibleAnywhere, Category = "Gargantua")
	TObjectPtr<UInstancedStaticMeshComponent> SlugMarkers;

	/** Blockout stand-ins for the rival slingers. */
	UPROPERTY(VisibleAnywhere, Category = "Gargantua")
	TObjectPtr<UInstancedStaticMeshComponent> TrainerMarkers;

	/** Doorways to the neighbouring caverns. */
	UPROPERTY(VisibleAnywhere, Category = "Gargantua")
	TObjectPtr<UInstancedStaticMeshComponent> ExitMarkers;

	UPROPERTY()
	TArray<TObjectPtr<UPointLightComponent>> PassageLights;
};
