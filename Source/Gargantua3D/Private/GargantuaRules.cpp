#include "GargantuaRules.h"

#include "Battle.h"
#include "Fusions.h"
#include "Policies.h"

DEFINE_LOG_CATEGORY(LogGargantua);

/**
 * An alias, deliberately not `using namespace`.
 *
 * Unreal compiles a module's .cpp files as one merged translation unit, so a
 * file-scope `using namespace` leaks into every file after it in the blob.
 * That made Gargantua::ElementCount visible unqualified and collided with a
 * local of the same name inside the engine's own mesh headers, which fails the
 * build in someone else's code for a reason that is entirely ours.
 */
namespace G = Gargantua;

namespace
{
	FGargSpecies ToBlueprint(const G::Species& S)
	{
		FGargSpecies Out;
		Out.Key = FName(S.Key.data());
		Out.Name = FText::FromString(FString(S.Name.data()));
		Out.Element = static_cast<EGargElement>(S.Element);
		Out.Role = static_cast<EGargRole>(S.Role);
		Out.Rarity = static_cast<EGargRarity>(S.RarityTier);
		Out.Power = S.Power;
		Out.Cooldown = S.Cooldown;
		Out.Flavor = FText::FromString(FString(S.Flavor.data()));
		Out.Color = FLinearColor::FromSRGBColor(FColor::FromHex(FString(G::ElementOf(S.Element).Color.data())));
		return Out;
	}

	FGargFusion ToBlueprint(const G::Fusion& F)
	{
		FGargFusion Out;
		Out.Name = FText::FromString(FString(F.Name.data()));
		Out.FirstKey = FName(G::SpeciesOf(F.A).Key.data());
		Out.SecondKey = FName(G::SpeciesOf(F.B).Key.data());
		Out.Power = F.Power;
		Out.Element = static_cast<EGargElement>(F.Element);
		Out.Description = FText::FromString(FString(F.Description.data()));
		return Out;
	}

	G::SpeciesId FromKey(FName Key)
	{
		const FString AsString = Key.ToString();
		return G::SpeciesFromKey(std::string_view(TCHAR_TO_UTF8(*AsString)));
	}
}

TArray<FGargSpecies> UGargantuaRules::GetAllSpecies()
{
	TArray<FGargSpecies> Out;
	Out.Reserve(Gargantua::SpeciesCount);
	for (const Gargantua::Species& S : Gargantua::AllSpecies)
	{
		Out.Add(ToBlueprint(S));
	}
	return Out;
}

bool UGargantuaRules::FindSpecies(FName Key, FGargSpecies& OutSpecies)
{
	const Gargantua::SpeciesId Id = FromKey(Key);
	if (Id == Gargantua::SpeciesId::None)
	{
		return false;
	}
	OutSpecies = ToBlueprint(Gargantua::SpeciesOf(Id));
	return true;
}

TArray<FGargFusion> UGargantuaRules::GetAllFusions()
{
	TArray<FGargFusion> Out;
	Out.Reserve(static_cast<int32>(Gargantua::AllFusions.size()));
	for (const Gargantua::Fusion& F : Gargantua::AllFusions)
	{
		Out.Add(ToBlueprint(F));
	}
	return Out;
}

bool UGargantuaRules::FindFusion(FName PreviousKey, FName NextKey, FGargFusion& OutFusion)
{
	const Gargantua::Fusion* Found = Gargantua::FindFusion(FromKey(PreviousKey), FromKey(NextKey));
	if (Found == nullptr)
	{
		return false;
	}
	OutFusion = ToBlueprint(*Found);
	return true;
}

float UGargantuaRules::GetMatchup(EGargElement Attacker, EGargElement Defender)
{
	return Gargantua::Matchup(static_cast<Gargantua::ElementId>(Attacker),
		static_cast<Gargantua::ElementId>(Defender));
}

void UGargantuaRules::GetWheelNeighbours(EGargElement Element, TArray<EGargElement>& OutStrongAgainst,
	TArray<EGargElement>& OutWeakAgainst)
{
	OutStrongAgainst.Reset();
	OutWeakAgainst.Reset();
	for (const Gargantua::ElementId Id : Gargantua::StrongAgainst(static_cast<Gargantua::ElementId>(Element)))
	{
		OutStrongAgainst.Add(static_cast<EGargElement>(Id));
	}
	for (const Gargantua::ElementId Id : Gargantua::WeakAgainst(static_cast<Gargantua::ElementId>(Element)))
	{
		OutWeakAgainst.Add(static_cast<EGargElement>(Id));
	}
}

FString UGargantuaRules::RunRulesSelfCheck()
{
	// The wheel, over all one hundred pairs. Antisymmetry is the property that
	// makes it fair; without it one element is quietly stronger than the rest.
	int32 Problems = 0;
	for (int32 A = 0; A < G::ElementCount; ++A)
	{
		int32 Strong = 0;
		int32 Weak = 0;
		for (int32 B = 0; B < G::ElementCount; ++B)
		{
			const float Forward = G::Matchup(static_cast<G::ElementId>(A), static_cast<G::ElementId>(B));
			const float Reverse = G::Matchup(static_cast<G::ElementId>(B), static_cast<G::ElementId>(A));
			if (A == B)
			{
				if (Forward != G::NeutralMultiplier) { ++Problems; }
				continue;
			}
			if (Forward == G::StrongMultiplier) { ++Strong; if (Reverse != G::WeakMultiplier) { ++Problems; } }
			else if (Forward == G::WeakMultiplier) { ++Weak; }
			else if (Reverse != G::NeutralMultiplier) { ++Problems; }
		}
		if (Strong != 2 || Weak != 2) { ++Problems; }
	}

	// A real duel, played by the two search policies, so this proves the engine
	// runs rather than merely that the headers compiled.
	const G::SpeciesId Mine[] = {G::SpeciesId::Cinderling, G::SpeciesId::Skirlwing, G::SpeciesId::Voltmote};
	const G::SpeciesId Theirs[] = {G::SpeciesId::Gritmaw, G::SpeciesId::Nullmoth, G::SpeciesId::Mirebubble};
	G::BattleState State = G::MakeBattle(G::MakeSide(Mine, 3), G::MakeSide(Theirs, 3));

	G::BattleLog Log;
	int32 Plies = 0;
	int32 Fusions = 0;
	while (!State.IsOver() && Plies < 400)
	{
		const G::Action Chosen = State.Active == 0 ? G::TacticalPolicy(State, 0, 4) : G::GreedyPolicy(State, 1);
		State = G::ApplyAction(State, Chosen, &Log);
		++Plies;
	}
	for (const G::BattleEvent& E : Log)
	{
		if (E.Type == G::EventType::Fire && E.Fused)
		{
			++Fusions;
		}
	}

	const FString Outcome = State.Drawn
		? TEXT("a draw")
		: FString::Printf(TEXT("side %d won"), State.Winner);

	const FString Report = FString::Printf(
		TEXT("Gargantua rules: %d species, %d fusions. Wheel problems: %d. ")
		TEXT("Duel finished on turn %d (%s), %d fusions fired."),
		G::SpeciesCount, static_cast<int32>(G::AllFusions.size()), Problems, State.Turn, *Outcome, Fusions);

	if (Problems == 0)
	{
		UE_LOG(LogGargantua, Display, TEXT("%s"), *Report);
	}
	else
	{
		UE_LOG(LogGargantua, Error, TEXT("%s"), *Report);
	}
	return Report;
}
