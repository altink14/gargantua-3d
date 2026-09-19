#pragma once

#include "CoreMinimal.h"

#include "Elements.h"
#include "Species.h"
#include "Types.h"

#include "GargantuaTypes.generated.h"

/**
 * Unreal-side mirrors of the rules enums.
 *
 * Core/ deliberately knows nothing about Unreal, so it cannot use UENUM. These
 * mirrors exist so designers can see and pick these values in Blueprints and
 * data assets, and every one is pinned to its Core counterpart with a
 * static_assert below. Add a slug to Core and forget to add it here and the
 * build fails — which is the only way a mirror stays honest.
 */

UENUM(BlueprintType)
enum class EGargElement : uint8
{
	Ember	UMETA(DisplayName = "Ember"),
	Bloom	UMETA(DisplayName = "Bloom"),
	Venom	UMETA(DisplayName = "Venom"),
	Stone	UMETA(DisplayName = "Stone"),
	Frost	UMETA(DisplayName = "Frost"),
	Spark	UMETA(DisplayName = "Spark"),
	Gale	UMETA(DisplayName = "Gale"),
	Tide	UMETA(DisplayName = "Tide"),
	Lumen	UMETA(DisplayName = "Lumen"),
	Umbra	UMETA(DisplayName = "Umbra"),
};

UENUM(BlueprintType)
enum class EGargRole : uint8
{
	Vanguard	UMETA(DisplayName = "Vanguard"),
	Burner		UMETA(DisplayName = "Burner"),
	Digger		UMETA(DisplayName = "Digger"),
	Exploder	UMETA(DisplayName = "Exploder"),
	Healer		UMETA(DisplayName = "Healer"),
	Trickster	UMETA(DisplayName = "Trickster"),
};

UENUM(BlueprintType)
enum class EGargRarity : uint8
{
	Common		UMETA(DisplayName = "Common"),
	Uncommon	UMETA(DisplayName = "Uncommon"),
	Rare		UMETA(DisplayName = "Rare"),
	Ultra		UMETA(DisplayName = "Ultra Rare"),
};

/** These keep the mirrors and the rules from drifting apart silently. */
static_assert(static_cast<uint8>(EGargElement::Umbra) == static_cast<uint8>(Gargantua::ElementId::Umbra),
	"EGargElement has drifted from Gargantua::ElementId");
static_assert(static_cast<uint8>(EGargRole::Trickster) == static_cast<uint8>(Gargantua::RoleId::Trickster),
	"EGargRole has drifted from Gargantua::RoleId");
static_assert(static_cast<uint8>(EGargRarity::Ultra) == static_cast<uint8>(Gargantua::Rarity::Ultra),
	"EGargRarity has drifted from Gargantua::Rarity");

/**
 * One slug, as the editor sees it.
 *
 * A view of the rules data rather than a second copy of it: the values are read
 * straight out of Core at runtime, so there is no table here to fall out of date.
 */
USTRUCT(BlueprintType)
struct GARGANTUA3D_API FGargSpecies
{
	GENERATED_BODY()

	/** Stable identity, and what a save file stores. */
	UPROPERTY(BlueprintReadOnly, Category = "Gargantua")
	FName Key;

	UPROPERTY(BlueprintReadOnly, Category = "Gargantua")
	FText Name;

	UPROPERTY(BlueprintReadOnly, Category = "Gargantua")
	EGargElement Element = EGargElement::Ember;

	UPROPERTY(BlueprintReadOnly, Category = "Gargantua")
	EGargRole Role = EGargRole::Burner;

	UPROPERTY(BlueprintReadOnly, Category = "Gargantua")
	EGargRarity Rarity = EGargRarity::Common;

	/** Base magnitude, 1..10. Scaled by the element wheel, never by chance. */
	UPROPERTY(BlueprintReadOnly, Category = "Gargantua")
	int32 Power = 0;

	/** Turns unavailable after firing, 1..4. */
	UPROPERTY(BlueprintReadOnly, Category = "Gargantua")
	int32 Cooldown = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Gargantua")
	FText Flavor;

	/** Element tint, for glow and creature colour. */
	UPROPERTY(BlueprintReadOnly, Category = "Gargantua")
	FLinearColor Color = FLinearColor::White;
};

/** One fusion: the pair that makes it and what it does. */
USTRUCT(BlueprintType)
struct GARGANTUA3D_API FGargFusion
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Gargantua")
	FText Name;

	UPROPERTY(BlueprintReadOnly, Category = "Gargantua")
	FName FirstKey;

	UPROPERTY(BlueprintReadOnly, Category = "Gargantua")
	FName SecondKey;

	UPROPERTY(BlueprintReadOnly, Category = "Gargantua")
	int32 Power = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Gargantua")
	EGargElement Element = EGargElement::Ember;

	/** One sentence a player can act on: what it does, not how it feels. */
	UPROPERTY(BlueprintReadOnly, Category = "Gargantua")
	FText Description;
};
