/**
 * The rules, checked.
 *
 * No test framework on purpose: this is a dependency-free core and its tests
 * should be too, so they run anywhere the engine does with one command.
 *
 * Every case here exists because something went wrong once. The purity and
 * determinism checks guard the property the whole search rests on; the bounds
 * checks came from an adversarial pass that drove shield to 899 and a cooldown
 * to 153; the unknown-species case came from a crash that reached the screen.
 */

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "Battle.h"
#include "Elements.h"
#include "Fusions.h"
#include "Policies.h"
#include "Species.h"

using namespace Gargantua;

namespace {

int Failures = 0;
int Checks = 0;

void Check(bool Condition, const char* What) {
    ++Checks;
    if (!Condition) {
        ++Failures;
        std::printf("  FAIL  %s\n", What);
    }
}

void Section(const char* Name) { std::printf("\n%s\n", Name); }

BattleState Battle(std::vector<SpeciesId> Mine, std::vector<SpeciesId> Theirs, int TheirHp = BaseHp) {
    return MakeBattle(MakeSide(Mine.data(), static_cast<int>(Mine.size())),
                      MakeSide(Theirs.data(), static_cast<int>(Theirs.size()), TheirHp));
}

/**
 * Compares two positions by value, field by field.
 *
 * Deliberately not memcmp: these structs contain padding between members, and
 * padding bytes are indeterminate after a copy. Comparing them compares
 * garbage and reports differences that do not exist.
 */
bool SameSide(const SideState& A, const SideState& B) {
    if (A.Hp != B.Hp || A.MaxHp != B.MaxHp || A.Shield != B.Shield) return false;
    if (A.RosterCount != B.RosterCount || A.LastFired != B.LastFired) return false;
    if (A.BurnCount != B.BurnCount) return false;
    for (int I = 0; I < A.RosterCount; ++I) {
        if (A.Roster[I].Id != B.Roster[I].Id) return false;
        if (A.Roster[I].CooldownRemaining != B.Roster[I].CooldownRemaining) return false;
    }
    for (int I = 0; I < A.BurnCount; ++I) {
        if (A.Burns[I].TurnsRemaining != B.Burns[I].TurnsRemaining) return false;
        if (A.Burns[I].DamagePerTurn != B.Burns[I].DamagePerTurn) return false;
    }
    return true;
}

bool SameState(const BattleState& A, const BattleState& B) {
    return A.Turn == B.Turn && A.Active == B.Active && A.Winner == B.Winner &&
           A.Drawn == B.Drawn && SameSide(A.Sides[0], B.Sides[0]) &&
           SameSide(A.Sides[1], B.Sides[1]);
}

const BattleEvent* LastOf(const BattleLog& Log, EventType Type) {
    for (auto It = Log.rbegin(); It != Log.rend(); ++It) {
        if (It->Type == Type) {
            return &*It;
        }
    }
    return nullptr;
}

void TestContract() {
    Section("the contract");

    {
        BattleState Before = Battle({SpeciesId::Cinderling}, {SpeciesId::Gritmaw});
        const BattleState Copy = Before;
        ApplyAction(Before, Action{ActionType::Fire, SpeciesId::Cinderling});
        Check(SameState(Before, Copy), "never mutates the state it was given");
    }

    {
        auto Run = [] {
            BattleState S = Battle({SpeciesId::Cinderling, SpeciesId::Skirlwing},
                                   {SpeciesId::Gritmaw, SpeciesId::Nullmoth});
            for (SpeciesId Id : {SpeciesId::Cinderling, SpeciesId::Gritmaw, SpeciesId::Skirlwing,
                                 SpeciesId::Nullmoth}) {
                S = ApplyAction(S, Action{ActionType::Fire, Id});
            }
            return S;
        };
        Check(SameState(Run(), Run()), "same actions always give the same result");
    }

    {
        // Voltmote recovers for 3 turns. Cinderling would be a bad choice: at
        // cooldown 1 it is ready again on the very next turn.
        BattleLog Log;
        BattleState S = Battle({SpeciesId::Voltmote, SpeciesId::Skirlwing}, {SpeciesId::Gritmaw});
        S = ApplyAction(S, Action{ActionType::Fire, SpeciesId::Voltmote}, &Log);
        S = ApplyAction(S, Action{ActionType::Fire, SpeciesId::Gritmaw}, &Log);
        const int Turn = S.Turn;
        S = ApplyAction(S, Action{ActionType::Fire, SpeciesId::Voltmote}, &Log);

        const BattleEvent* Illegal = LastOf(Log, EventType::Illegal);
        Check(Illegal != nullptr, "an illegal move is explained, not thrown");
        Check(S.Turn == Turn && S.Active == 0, "an illegal move does not spend the turn");
    }

    {
        BattleLog Log;
        BattleState S = Battle({SpeciesId::Cinderling}, {SpeciesId::Gritmaw});
        S = ApplyAction(S, Action{ActionType::Fire, SpeciesId::Pyrelash}, &Log);
        Check(LastOf(Log, EventType::Illegal) != nullptr, "refuses a slug not in the loadout");
        Check(S.Turn == 0, "refusing does not spend the turn");
    }

    {
        // The browser build crashed here: an id the content did not know reached
        // the species table unguarded.
        BattleLog Log;
        BattleState S = Battle({SpeciesId::Cinderling}, {SpeciesId::Gritmaw});
        S = ApplyAction(S, Action{ActionType::Fire, SpeciesId::None}, &Log);
        Check(LastOf(Log, EventType::Illegal) != nullptr, "an unknown species is explained, not a crash");
    }
}

void TestBalanceRules() {
    Section("rules the balance numbers depend on");

    Check(Matchup(ElementId::Ember, ElementId::Bloom) == StrongMultiplier, "a strong matchup doubles");
    Check(Matchup(ElementId::Bloom, ElementId::Ember) == WeakMultiplier, "and the reverse is halved");
    Check(Matchup(ElementId::Ember, ElementId::Frost) == NeutralMultiplier, "distant elements are even");

    {
        // Nullmoth is a vanguard: power 5, so 5 shield and no damage.
        // Thornwhistle is a healer and deals nothing, so any shield lost is decay.
        BattleLog Log;
        BattleState S = Battle({SpeciesId::Nullmoth, SpeciesId::Cinderling}, {SpeciesId::Thornwhistle});
        S = ApplyAction(S, Action{ActionType::Fire, SpeciesId::Nullmoth}, &Log);
        Check(S.Sides[0].Shield == 5, "a vanguard raises shield equal to its power");
        S = ApplyAction(S, Action{ActionType::Fire, SpeciesId::Thornwhistle}, &Log);
        Check(S.Sides[0].Shield == 3, "shield fades by 2 at the start of its owner's turn");
        Check(LastOf(Log, EventType::ShieldFade) != nullptr, "and says so");
    }

    {
        BattleState S = Battle({SpeciesId::Nullmoth}, {SpeciesId::Thornwhistle});
        S = ApplyAction(S, Action{ActionType::Fire, SpeciesId::Nullmoth});
        for (int I = 0; I < 8; ++I) {
            S = ApplyAction(S, Action{ActionType::Hold});
        }
        Check(S.Sides[0].Shield == 0, "shield never fades below zero");
    }

    {
        BattleState S = Battle({SpeciesId::Cinderling, SpeciesId::Skirlwing}, {SpeciesId::Gritmaw});
        S = ApplyAction(S, Action{ActionType::Fire, SpeciesId::Cinderling});
        Check(S.Sides[0].LastFired == SpeciesId::Cinderling, "firing sets your stance");
        Check(ResolveShot(S, 0, SpeciesId::Skirlwing).Fused, "which sets up a fusion");

        S = ApplyAction(S, Action{ActionType::Fire, SpeciesId::Gritmaw});
        S = ApplyAction(S, Action{ActionType::Hold});
        Check(S.Sides[0].LastFired == SpeciesId::None, "holding clears your stance");
    }

    {
        BattleState A = Battle({SpeciesId::Cinderling, SpeciesId::Skirlwing}, {SpeciesId::Gritmaw});
        A = ApplyAction(A, Action{ActionType::Fire, SpeciesId::Cinderling});
        BattleState B = Battle({SpeciesId::Cinderling, SpeciesId::Skirlwing}, {SpeciesId::Gritmaw});
        B = ApplyAction(B, Action{ActionType::Fire, SpeciesId::Skirlwing});
        Check(ResolveShot(A, 0, SpeciesId::Skirlwing).Name == "Firestorm" &&
                  ResolveShot(B, 0, SpeciesId::Cinderling).Name == "Firestorm",
              "a fusion resolves in either order");
    }

    {
        // Both shots branch from ONE position, so the defender's last-fired
        // element is identical for each and the comparison is fair.
        BattleState Base = Battle({SpeciesId::Gritmaw, SpeciesId::Voltmote}, {SpeciesId::Nullmoth});
        Base = ApplyAction(Base, Action{ActionType::Hold});
        Base = ApplyAction(Base, Action{ActionType::Fire, SpeciesId::Nullmoth});
        Check(Base.Sides[1].Shield == 5 && Base.Active == 0, "the opponent has a shield up");

        const BattleState Dug = ApplyAction(Base, Action{ActionType::Fire, SpeciesId::Gritmaw});
        const BattleState Hit = ApplyAction(Base, Action{ActionType::Fire, SpeciesId::Voltmote});

        // The digger absorbs nothing, but shield still drops by the 2 it fades
        // at the start of their turn.
        Check(Dug.Sides[1].Shield == 3, "a digger's shot is not absorbed");
        Check(Hit.Sides[1].Shield < 3, "anything else is absorbed first");
        Check((Base.Sides[1].Hp - Hit.Sides[1].Hp) < (Base.Sides[1].Hp - Dug.Sides[1].Hp),
              "so the digger lands more of its damage");
    }

    {
        BattleState S = MakeBattle(MakeSide(std::vector<SpeciesId>{SpeciesId::Pyrelash}.data(), 1),
                                   MakeSide(std::vector<SpeciesId>{SpeciesId::Thornwhistle}.data(), 1, 4));
        S = ApplyAction(S, Action{ActionType::Fire, SpeciesId::Pyrelash});
        Check(S.Winner == 0 && S.Sides[1].Hp == 0, "the match ends the moment health runs out");

        const BattleState After = ApplyAction(S, Action{ActionType::Fire, SpeciesId::Pyrelash});
        Check(SameState(After, S), "a finished battle ignores further actions");
    }
}

void TestBounds() {
    Section("nothing grows without limit");

    {
        // Two teams that cannot damage each other ran forever in the browser
        // build, with no turn cap and no way out of the screen.
        BattleState S = Battle({SpeciesId::Dewspine}, {SpeciesId::Thornwhistle}, 26);
        int Plies = 0;
        while (!S.IsOver() && Plies < 600) {
            S = ApplyAction(S, TacticalPolicy(S, S.Active, 3));
            ++Plies;
        }
        Check(S.Drawn, "a duel nobody can win ends in a draw");
        Check(S.Turn == MaxTurns, "at the turn cap");
    }

    {
        BattleState S = Battle({SpeciesId::Bramblehusk, SpeciesId::Dewspine, SpeciesId::Nullmoth},
                               {SpeciesId::Brinemoth, SpeciesId::Wispwick, SpeciesId::Thornwhistle});
        int Plies = 0;
        int PeakShield = 0;
        int PeakCooldown = 0;
        while (!S.IsOver() && Plies < 400) {
            S = ApplyAction(S, TacticalPolicy(S, S.Active, 3));
            for (const SideState& Side : S.Sides) {
                PeakShield = std::max(PeakShield, Side.Shield);
                for (int I = 0; I < Side.RosterCount; ++I) {
                    PeakCooldown = std::max(PeakCooldown, Side.Roster[I].CooldownRemaining);
                }
            }
            ++Plies;
        }
        Check(PeakShield <= MaxShield, "shield stays bounded");
        Check(PeakCooldown <= MaxCooldown, "cooldown stays bounded");
    }

    {
        // A depth of zero or a negative never reaches the base case. Trainer
        // depth is authored content, so a typo must not hang the game.
        const BattleState S = Battle({SpeciesId::Cinderling, SpeciesId::Voltmote}, {SpeciesId::Gritmaw});
        for (int D : {0, -1, -999, 99}) {
            const Action A = TacticalPolicy(S, 0, D);
            Check(A.Type == ActionType::Fire || A.Type == ActionType::Hold,
                  "a hostile search depth still returns a legal move");
        }
    }
}

void TestWheel() {
    Section("the element wheel");

    bool Total = true;
    bool Antisymmetric = true;
    bool Balanced = true;

    for (int A = 0; A < ElementCount; ++A) {
        int Strong = 0;
        int Weak = 0;
        int Neutral = 0;
        for (int B = 0; B < ElementCount; ++B) {
            const float F = Matchup(static_cast<ElementId>(A), static_cast<ElementId>(B));
            const float R = Matchup(static_cast<ElementId>(B), static_cast<ElementId>(A));
            if (F != StrongMultiplier && F != WeakMultiplier && F != NeutralMultiplier) {
                Total = false;
            }
            if (A != B) {
                if (F == StrongMultiplier && R != WeakMultiplier) Antisymmetric = false;
                if (F == NeutralMultiplier && R != NeutralMultiplier) Antisymmetric = false;
                if (F == StrongMultiplier) ++Strong;
                else if (F == WeakMultiplier) ++Weak;
                else ++Neutral;
            } else if (F != NeutralMultiplier) {
                Balanced = false;
            }
        }
        if (Strong != 2 || Weak != 2 || Neutral != 5) {
            Balanced = false;
        }
    }

    Check(Total, "every pair yields exactly one of the three multipliers");
    Check(Antisymmetric, "if a beats b then b is weak to a");
    Check(Balanced, "every element beats two, loses to two, ties five");

    bool HelpersAgree = true;
    for (int A = 0; A < ElementCount; ++A) {
        const auto Id = static_cast<ElementId>(A);
        for (ElementId B : StrongAgainst(Id)) {
            if (Matchup(Id, B) != StrongMultiplier) HelpersAgree = false;
        }
        for (ElementId B : WeakAgainst(Id)) {
            if (Matchup(Id, B) != WeakMultiplier) HelpersAgree = false;
        }
    }
    Check(HelpersAgree, "the codex helpers agree with the wheel");
}

void TestContent() {
    Section("content integrity");

    Check(AllSpecies.size() == static_cast<std::size_t>(SpeciesCount), "every species slot is filled");

    bool IdsMatchOrder = true;
    bool KeysRoundTrip = true;
    for (int I = 0; I < SpeciesCount; ++I) {
        if (static_cast<int>(AllSpecies[I].Id) != I) IdsMatchOrder = false;
        if (SpeciesFromKey(AllSpecies[I].Key) != AllSpecies[I].Id) KeysRoundTrip = false;
    }
    Check(IdsMatchOrder, "the species table is indexed by its own ids");
    Check(KeysRoundTrip, "every saved key maps back to its species");
    Check(SpeciesFromKey("no-such-slug") == SpeciesId::None, "an unknown key is not a species");

    bool NoSelfFusions = true;
    bool NoDuplicatePairs = true;
    for (std::size_t I = 0; I < AllFusions.size(); ++I) {
        if (AllFusions[I].A == AllFusions[I].B) NoSelfFusions = false;
        for (std::size_t J = I + 1; J < AllFusions.size(); ++J) {
            const bool Same = (AllFusions[I].A == AllFusions[J].A && AllFusions[I].B == AllFusions[J].B) ||
                              (AllFusions[I].A == AllFusions[J].B && AllFusions[I].B == AllFusions[J].A);
            if (Same) NoDuplicatePairs = false;
        }
    }
    Check(NoSelfFusions, "no slug fuses with itself");
    Check(NoDuplicatePairs, "no pair has two fusions shadowing each other");

    bool PowerAndCooldownSane = true;
    for (const Species& S : AllSpecies) {
        if (S.Power < 1 || S.Power > 10) PowerAndCooldownSane = false;
        if (S.Cooldown < 1 || S.Cooldown > 4) PowerAndCooldownSane = false;
    }
    Check(PowerAndCooldownSane, "power is 1..10 and cooldown 1..4 for every slug");

    // Density is what the depth proof rests on. Eight fusions across twelve
    // species was not enough, and the game failed its own test because of it.
    int Participating = 0;
    for (const Species& S : AllSpecies) {
        for (const Fusion& F : AllFusions) {
            if (F.A == S.Id || F.B == S.Id) {
                ++Participating;
                break;
            }
        }
    }
    Check(Participating == SpeciesCount, "every species takes part in at least one fusion");
}

}  // namespace

int main() {
    std::printf("Gargantua core tests — %d species, %d fusions\n", SpeciesCount,
                static_cast<int>(AllFusions.size()));

    TestContract();
    TestBalanceRules();
    TestBounds();
    TestWheel();
    TestContent();

    std::printf("\n%d checks, %d failures\n", Checks, Failures);
    if (Failures == 0) {
        std::printf("ALL CLEAR\n");
    }
    return Failures == 0 ? 0 : 1;
}
