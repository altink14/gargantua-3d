#include "GargantuaRules.h"

#include "Battle.h"
#include "Fusions.h"
#include "Policies.h"

DEFINE_LOG_CATEGORY(LogGargantua);

namespace
{
	using namespace Gargantua;

	FGargSpecies ToBlueprint(const Species& S)
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
		Out.Color = FLinearColor::FromSRGBColor(FColor::FromHex(FString(ElementOf(S.Element).Color.data())));
		return Out;
	}

	FGargFusion ToBlueprint(const Fusion& F)
	{
		FGargFusion Out;
		Out.Name = FText::FromString(FString(F.Name.data()));
		Out.FirstKey = FName(SpeciesOf(F.A).Key.data());
		Out.SecondKey = FName(SpeciesOf(F.B).Key.data());
		Out.Power = F.Power;
		Out.Element = static_cast<EGargElement>(F.Element);
		Out.Description = FText::FromString(FString(F.Description.data()));
		return Out;
	}

	SpeciesId FromKey(FName Key)
	{
		const FString AsString = Key.ToString();
		return SpeciesFromKey(std::string_view(TCHAR_TO_UTF8(*AsString)));
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
	using namespace Gargantua;

	// The wheel, over all one hundred pairs. Antisymmetry is the property that
	// makes it fair; without it one element is quietly stronger than the rest.
	int32 Problems = 0;
	for (int32 A = 0; A < ElementCount; ++A)
	{
		int32 Strong = 0;
		int32 Weak = 0;
		for (int32 B = 0; B < ElementCount; ++B)
		{
			const float Forward = Matchup(static_cast<ElementId>(A), static_cast<ElementId>(B));
			const float Reverse = Matchup(static_cast<ElementId>(B), static_cast<ElementId>(A));
			if (A == B)
			{
				if (Forward != NeutralMultiplier) { ++Problems; }
				continue;
			}
			if (Forward == StrongMultiplier) { ++Strong; if (Reverse != WeakMultiplier) { ++Problems; } }
			else if (Forward == WeakMultiplier) { ++Weak; }
			else if (Reverse != NeutralMultiplier) { ++Problems; }
		}
		if (Strong != 2 || Weak != 2) { ++Problems; }
	}

	// A real duel, played by the two search policies, so this proves the engine
	// runs rather than merely that the headers compiled.
	const SpeciesId Mine[] = {SpeciesId::Cinderling, SpeciesId::Skirlwing, SpeciesId::Voltmote};
	const SpeciesId Theirs[] = {SpeciesId::Gritmaw, SpeciesId::Nullmoth, SpeciesId::Mirebubble};
	BattleState State = MakeBattle(MakeSide(Mine, 3), MakeSide(Theirs, 3));

	BattleLog Log;
	int32 Plies = 0;
	int32 Fusions = 0;
	while (!State.IsOver() && Plies < 400)
	{
		const Action Chosen = State.Active == 0 ? TacticalPolicy(State, 0, 4) : GreedyPolicy(State, 1);
		State = ApplyAction(State, Chosen, &Log);
		++Plies;
	}
	for (const BattleEvent& E : Log)
	{
		if (E.Type == EventType::Fire && E.Fused)
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
		SpeciesCount, static_cast<int32>(AllFusions.size()), Problems, State.Turn, *Outcome, Fusions);

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
