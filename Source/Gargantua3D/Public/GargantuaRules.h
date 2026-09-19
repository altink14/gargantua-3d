#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "GargantuaTypes.h"

#include "GargantuaRules.generated.h"

GARGANTUA3D_API DECLARE_LOG_CATEGORY_EXTERN(LogGargantua, Log, All);

/**
 * The rules, as Unreal and Blueprints see them.
 *
 * Every function here reads straight out of Core/ rather than keeping its own
 * copy, so the numbers a designer sees in the editor are the same ones the
 * balance harness measured. There is no second source of truth to drift.
 */
UCLASS()
class GARGANTUA3D_API UGargantuaRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Every slug in the game, in codex order. */
	UFUNCTION(BlueprintPure, Category = "Gargantua|Content")
	static TArray<FGargSpecies> GetAllSpecies();

	/** One slug by its saved key. Returns false for an id the game does not have. */
	UFUNCTION(BlueprintPure, Category = "Gargantua|Content")
	static bool FindSpecies(FName Key, FGargSpecies& OutSpecies);

	/** Every fusion, including which pair produces it. */
	UFUNCTION(BlueprintPure, Category = "Gargantua|Content")
	static TArray<FGargFusion> GetAllFusions();

	/**
	 * The fusion produced by firing Next immediately after Previous, if any.
	 * Order never matters, so both arrangements are checked.
	 */
	UFUNCTION(BlueprintPure, Category = "Gargantua|Content")
	static bool FindFusion(FName PreviousKey, FName NextKey, FGargFusion& OutFusion);

	/**
	 * Damage multiplier for one element hitting another: 2, 1 or 0.5.
	 *
	 * A shot is scored against what the DEFENDER last fired, so the slug you
	 * pick also decides what you are exposed to next turn.
	 */
	UFUNCTION(BlueprintPure, Category = "Gargantua|Rules")
	static float GetMatchup(EGargElement Attacker, EGargElement Defender);

	/** The two elements this one beats, and the two that beat it. */
	UFUNCTION(BlueprintPure, Category = "Gargantua|Rules")
	static void GetWheelNeighbours(EGargElement Element, TArray<EGargElement>& OutStrongAgainst,
		TArray<EGargElement>& OutWeakAgainst);

	/**
	 * Runs the rules inside the engine and reports what happened.
	 *
	 * Checks the wheel's invariants over all one hundred element pairs, then
	 * plays a real duel between the two search policies and reports the result.
	 * It exists so that "the rules work in Unreal" is something observed on
	 * startup rather than assumed because the module compiled.
	 */
	UFUNCTION(BlueprintCallable, Category = "Gargantua|Diagnostics")
	static FString RunRulesSelfCheck();
};
