#include "GargCavern.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GargantuaRules.h"

#include "Caverns.h"
#include "World.h"

namespace G = Gargantua;

namespace
{
	/** The engine's unit cube: 100cm on a side, centred on its origin. */
	constexpr const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");

	/**
	 * Layout space to world space.
	 *
	 * The layouts were authored top-down, so their Y runs down the screen. Here
	 * it runs along Unreal's Y and the floor sits at Z zero. One scale constant,
	 * declared in World.h, is the only opinion about how big a cavern is.
	 */
	FVector ToWorld(float X, float Y, float Z = 0.0f)
	{
		return FVector(X * G::UnitsToCentimetres, Y * G::UnitsToCentimetres, Z);
	}

	const G::Biome* Lookup(FName Id)
	{
		const FString AsString = Id.ToString();
		return G::FindBiome(std::string_view(TCHAR_TO_UTF8(*AsString)));
	}

	UInstancedStaticMeshComponent* MakeInstances(AActor* Owner, USceneComponent* Parent,
		const TCHAR* Name, UStaticMesh* Mesh, bool bCollides)
	{
		UInstancedStaticMeshComponent* Comp =
			Owner->CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Comp->SetupAttachment(Parent);
		Comp->SetStaticMesh(Mesh);
		Comp->SetMobility(EComponentMobility::Static);
		Comp->SetCollisionEnabled(bCollides ? ECollisionEnabled::QueryAndPhysics
											: ECollisionEnabled::NoCollision);
		return Comp;
	}
}

AGargCavern::AGargCavern()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(CubePath);
	UStaticMesh* Cube = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;

	Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
	Floor->SetupAttachment(Root);
	Floor->SetStaticMesh(Cube);
	Floor->SetMobility(EComponentMobility::Static);

	Walls = MakeInstances(this, Root, TEXT("Walls"), Cube, true);
	// Markers are stand-ins you walk into, not obstacles you bump against, so
	// they are visible but do not block. The duel triggers on proximity.
	SlugMarkers = MakeInstances(this, Root, TEXT("SlugMarkers"), Cube, false);
	TrainerMarkers = MakeInstances(this, Root, TEXT("TrainerMarkers"), Cube, false);
	ExitMarkers = MakeInstances(this, Root, TEXT("ExitMarkers"), Cube, false);
}

void AGargCavern::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void AGargCavern::BeginPlay()
{
	Super::BeginPlay();

	const G::Biome* Biome = Lookup(BiomeId);
	if (Biome == nullptr)
	{
		UE_LOG(LogGargantua, Error, TEXT("No cavern named '%s'."), *BiomeId.ToString());
		return;
	}
	UE_LOG(LogGargantua, Display,
		TEXT("%hs built: %d walls, %d slugs, %d slingers, %d doorways, %.0fm across."),
		Biome->Name.data(), Biome->WallCount, Biome->EncounterCount, Biome->TrainerCount,
		Biome->ExitCount, Biome->Bounds.W * G::UnitsToCentimetres / 100.0f);
}

FVector AGargCavern::GetArrivalLocation() const
{
	const G::Biome* Biome = Lookup(BiomeId);
	if (Biome == nullptr)
	{
		return GetActorLocation();
	}

	// The first cavern has a fixed start; the others are entered through a door,
	// so the arrival point is wherever that door puts you.
	float X = 400.0f;
	float Y = 300.0f;
	if (Biome->Id != G::FirstBiome.Id)
	{
		for (const G::Biome* Other : G::AllBiomes)
		{
			for (int I = 0; I < Other->ExitCount; ++I)
			{
				if (Other->Exits[I].To == Biome->Id)
				{
					X = Other->Exits[I].Entry.X;
					Y = Other->Exits[I].Entry.Y;
				}
			}
		}
	}
	// Lifted clear of the floor so the capsule settles rather than starting
	// intersected with it.
	return GetActorLocation() + ToWorld(X, Y, 120.0f);
}

void AGargCavern::Rebuild()
{
	const G::Biome* Biome = Lookup(BiomeId);
	if (Biome == nullptr)
	{
		return;
	}

	Walls->ClearInstances();
	SlugMarkers->ClearInstances();
	TrainerMarkers->ClearInstances();
	ExitMarkers->ClearInstances();

	const float S = G::UnitsToCentimetres;

	// The floor, one stretched cube sitting just under zero so the walkable
	// surface is exactly Z=0.
	const FVector FloorSize(Biome->Bounds.W * S, Biome->Bounds.H * S, 20.0f);
	Floor->SetRelativeLocation(FVector(FloorSize.X * 0.5f, FloorSize.Y * 0.5f, -10.0f));
	Floor->SetRelativeScale3D(FloorSize / 100.0f);

	for (int I = 0; I < Biome->WallCount; ++I)
	{
		const G::Rect& R = Biome->Walls[I];
		const FVector Size(R.W * S, R.H * S, WallHeight);
		const FVector Centre((R.X + R.W * 0.5f) * S, (R.Y + R.H * 0.5f) * S, WallHeight * 0.5f);
		Walls->AddInstance(FTransform(FRotator::ZeroRotator, Centre, Size / 100.0f));
	}

	// Slugs are small and low. A blockout, but a legible one: you should be able
	// to tell a slug from a slinger from a doorway across a dark room.
	for (int I = 0; I < Biome->EncounterCount; ++I)
	{
		const G::Encounter& E = Biome->Encounters[I];
		SlugMarkers->AddInstance(FTransform(FRotator::ZeroRotator,
			ToWorld(E.X, E.Y, 40.0f), FVector(0.8f, 0.8f, 0.6f)));
	}

	// Slingers stand upright and are taller than the player.
	for (int I = 0; I < Biome->TrainerCount; ++I)
	{
		const G::Trainer& T = Biome->Trainers[I];
		TrainerMarkers->AddInstance(FTransform(FRotator::ZeroRotator,
			ToWorld(T.X, T.Y, 110.0f), FVector(0.7f, 0.7f, 2.2f)));
	}

	// Doorways are the size of the area you walk into to travel.
	for (int I = 0; I < Biome->ExitCount; ++I)
	{
		const G::Rect& B = Biome->Exits[I].Bounds;
		const FVector Size(B.W * S, B.H * S, WallHeight * 0.8f);
		const FVector Centre((B.X + B.W * 0.5f) * S, (B.Y + B.H * 0.5f) * S, Size.Z * 0.5f);
		ExitMarkers->AddInstance(FTransform(FRotator::ZeroRotator, Centre, Size / 100.0f));
	}

	// A lamp in the mouth of every gap.
	//
	// These are not decoration. In the browser build the single worst thing
	// about walking a cavern was that you could not see which gaps in a wall
	// were passages, and lighting them was what fixed it. The same information
	// problem exists in 3D, so the same lights are placed from the same data.
	for (UPointLightComponent* Light : PassageLights)
	{
		if (Light != nullptr)
		{
			Light->DestroyComponent();
		}
	}
	PassageLights.Reset();

	for (int I = 0; I < Biome->PassageCount; ++I)
	{
		const G::Point& P = Biome->Passages[I];
		UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
		Light->SetupAttachment(Root);
		Light->RegisterComponent();
		Light->SetRelativeLocation(ToWorld(P.X, P.Y, 250.0f));
		Light->SetIntensity(9000.0f);
		Light->SetAttenuationRadius(1400.0f);
		Light->SetLightColor(FLinearColor(1.0f, 0.78f, 0.42f));
		Light->SetMobility(EComponentMobility::Static);
		PassageLights.Add(Light);
	}
}
