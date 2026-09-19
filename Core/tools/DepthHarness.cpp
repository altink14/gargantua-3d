/**
 * Does Gargantua reward thinking, or is it a slot machine with slugs?
 *
 * This harness answers that with numbers instead of opinion. The four
 * thresholds below were fixed before the combat system was written, and they
 * are not to be lowered to make a run pass; a failure here is a signal to
 * redesign the combat, not to edit this file.
 *
 * The same four thresholds pass in the browser build this was ported from. If
 * they pass here too, the design survived the move to C++ — which is the whole
 * reason this runs before any 3D work begins.
 *
 * Build with Core\build.bat, then run build\depth.exe [matches].
 */

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

#include "Battle.h"
#include "Policies.h"
#include "Species.h"

using namespace Gargantua;

namespace {

constexpr int LoadoutSize = 5;
constexpr int SearchDepth = 4;
/** A match this long is a stalemate, not a win for anyone. */
constexpr int MaxActions = 400;
/**
 * Depth-6 search is expensive, so that duel runs on its own sample size.
 *
 * The browser build capped this at 500 because JavaScript made anything more
 * take minutes. In C++ the entire harness runs in seconds, so there is no
 * reason to measure a ~60% effect with a sample whose noise is +-2 points —
 * twenty times the margin being judged. The threshold is unchanged; only the
 * confidence in the number is.
 */
constexpr int DeepDuelMatches = 4000;

struct Thresholds {
    float TacticalOverGreedy = 0.70f;
    float GreedyOverRandom = 0.70f;
    float MaxSpeciesShare = 0.55f;
    float DeepOverShallow = 0.60f;
};
constexpr Thresholds Bar{};

using Policy = std::function<Action(const BattleState&, int)>;

std::vector<SpeciesId> PickLoadout(Rng& Generator) {
    std::vector<SpeciesId> Pool;
    Pool.reserve(SpeciesCount);
    for (int I = 0; I < SpeciesCount; ++I) {
        Pool.push_back(static_cast<SpeciesId>(I));
    }
    for (int I = static_cast<int>(Pool.size()) - 1; I > 0; --I) {
        std::swap(Pool[I], Pool[Generator.Below(I + 1)]);
    }
    Pool.resize(LoadoutSize);
    return Pool;
}

struct Outcome {
    int Winner = -1;  // -1 for a draw or stalemate
};

Outcome PlayMatch(const Policy& A, const Policy& B, const std::vector<SpeciesId>& LoadA,
                  const std::vector<SpeciesId>& LoadB) {
    BattleState State = MakeBattle(MakeSide(LoadA.data(), static_cast<int>(LoadA.size())),
                                   MakeSide(LoadB.data(), static_cast<int>(LoadB.size())));
    for (int I = 0; I < MaxActions && !State.IsOver(); ++I) {
        const Policy& Mover = State.Active == 0 ? A : B;
        State = ApplyAction(State, Mover(State, State.Active));
    }
    return Outcome{State.Winner};
}

struct DuelResult {
    int ChallengerWins = 0;
    int IncumbentWins = 0;
    int Draws = 0;
    float Rate = 0.0f;
    std::vector<std::vector<SpeciesId>> WinningLoadouts;
};

/**
 * Plays every pairing twice with the sides swapped.
 *
 * Whoever moves first gets a free shot, so measuring a policy on one side only
 * would report that advantage as skill.
 */
DuelResult Duel(const Policy& Challenger, const Policy& Incumbent, int Matches) {
    DuelResult R{};
    Rng Generator(0xC0FFEEu);

    for (int I = 0; I < Matches; ++I) {
        const std::vector<SpeciesId> LoadA = PickLoadout(Generator);
        const std::vector<SpeciesId> LoadB = PickLoadout(Generator);
        const bool ChallengerFirst = (I % 2) == 0;

        const Outcome Result = ChallengerFirst ? PlayMatch(Challenger, Incumbent, LoadA, LoadB)
                                               : PlayMatch(Incumbent, Challenger, LoadA, LoadB);

        if (Result.Winner < 0) {
            ++R.Draws;
            continue;
        }
        const bool ChallengerWon = ChallengerFirst ? Result.Winner == 0 : Result.Winner == 1;
        if (ChallengerWon) {
            ++R.ChallengerWins;
        } else {
            ++R.IncumbentWins;
        }
        R.WinningLoadouts.push_back(Result.Winner == 0 ? LoadA : LoadB);
    }

    const int Decided = R.ChallengerWins + R.IncumbentWins;
    R.Rate = Decided == 0 ? 0.0f : static_cast<float>(R.ChallengerWins) / static_cast<float>(Decided);
    return R;
}

std::string Pct(float X) {
    char Buffer[16];
    std::snprintf(Buffer, sizeof(Buffer), "%.1f%%", X * 100.0f);
    return Buffer;
}

}  // namespace

int main(int argc, char** argv) {
    const int Matches = argc > 1 ? std::atoi(argv[1]) : 2000;
    const auto Started = std::chrono::steady_clock::now();

    std::printf("Gargantua depth harness — %d matches per duel, search depth %d\n", Matches,
                SearchDepth);
    std::printf("This runs a lot of games. At the default 2000 it takes a few minutes.\n\n");

    Rng RandomGen(0xBEEFu);
    const Policy Tactical = [](const BattleState& S, int Side) {
        return TacticalPolicy(S, Side, SearchDepth);
    };
    const Policy Greedy = [](const BattleState& S, int Side) { return GreedyPolicy(S, Side); };
    const Policy Random = [&RandomGen](const BattleState& S, int Side) {
        return RandomPolicy(S, Side, RandomGen);
    };

    const DuelResult T1 = Duel(Tactical, Greedy, Matches);
    std::printf("1. Tactical beats greedy   %6s  (%d-%d, %d draws)\n", Pct(T1.Rate).c_str(),
                T1.ChallengerWins, T1.IncumbentWins, T1.Draws);

    const DuelResult T2 = Duel(Greedy, Random, Matches);
    std::printf("2. Greedy beats random     %6s  (%d-%d, %d draws)\n", Pct(T2.Rate).c_str(),
                T2.ChallengerWins, T2.IncumbentWins, T2.Draws);

    std::array<int, SpeciesCount> Usage{};
    int TotalWinning = 0;
    for (const DuelResult* R : {&T1, &T2}) {
        for (const std::vector<SpeciesId>& Load : R->WinningLoadouts) {
            ++TotalWinning;
            std::array<bool, SpeciesCount> Seen{};
            for (SpeciesId Id : Load) {
                const int Idx = static_cast<int>(Id);
                if (!Seen[Idx]) {
                    Seen[Idx] = true;
                    ++Usage[Idx];
                }
            }
        }
    }

    int TopIdx = 0;
    for (int I = 1; I < SpeciesCount; ++I) {
        if (Usage[I] > Usage[TopIdx]) {
            TopIdx = I;
        }
    }
    const float TopShare = TotalWinning == 0
                               ? 0.0f
                               : static_cast<float>(Usage[TopIdx]) / static_cast<float>(TotalWinning);
    std::printf("3. Most-used species       %6s  (%s)\n", Pct(TopShare).c_str(),
                std::string(AllSpecies[TopIdx].Key).c_str());

    /**
     * Threshold 4: does looking further ahead keep paying?
     *
     * A game has strategic depth when a deeper search keeps converting into
     * wins. In a shallow game the extra plies buy nothing, because everything
     * that matters is already visible one move out.
     *
     * This replaced an earlier metric — "median viable moves >= 3" — which
     * measured whether a depth-4 search finds a single clearly best move.
     * That is nearly always true in any deterministic game, chess included, so
     * it was a bad proxy rather than a demanding one. Changed once, with the
     * project owner's sign-off, and recorded here so the history is plain.
     */
    const Policy Deep = [](const BattleState& S, int Side) { return TacticalPolicy(S, Side, 6); };
    const Policy Shallow = [](const BattleState& S, int Side) { return TacticalPolicy(S, Side, 2); };
    const int DeepMatches = std::min(Matches, DeepDuelMatches);
    const DuelResult T4 = Duel(Deep, Shallow, DeepMatches);
    std::printf("4. Depth-6 beats depth-2   %6s  (%d-%d, %d draws, %d matches)\n",
                Pct(T4.Rate).c_str(), T4.ChallengerWins, T4.IncumbentWins, T4.Draws, DeepMatches);

    const float Baseline = static_cast<float>(LoadoutSize) / static_cast<float>(SpeciesCount);
    std::printf("\nSpecies share of winning loadouts (even share would be %s):\n",
                Pct(Baseline).c_str());
    std::array<int, SpeciesCount> Order{};
    for (int I = 0; I < SpeciesCount; ++I) {
        Order[I] = I;
    }
    std::sort(Order.begin(), Order.end(), [&](int A, int B) { return Usage[A] > Usage[B]; });
    for (int I : Order) {
        const float Share =
            TotalWinning == 0 ? 0.0f : static_cast<float>(Usage[I]) / static_cast<float>(TotalWinning);
        std::printf("   %-14s %s\n", std::string(AllSpecies[I].Key).c_str(), Pct(Share).c_str());
    }

    struct Line {
        const char* Label;
        bool Pass;
        std::string Got;
    };
    const std::array<Line, 4> Results{{
        {"Tactical beats greedy >= 70%", T1.Rate >= Bar.TacticalOverGreedy, Pct(T1.Rate)},
        {"Greedy beats random >= 70%", T2.Rate >= Bar.GreedyOverRandom, Pct(T2.Rate)},
        {"No species above 55% of wins", TopShare <= Bar.MaxSpeciesShare, Pct(TopShare)},
        {"Depth-6 beats depth-2 >= 60%", T4.Rate >= Bar.DeepOverShallow, Pct(T4.Rate)},
    }};

    std::printf("\n--- Locked thresholds ---\n");
    int Failed = 0;
    for (const Line& L : Results) {
        std::printf("%s  %-32s got %s\n", L.Pass ? "PASS" : "FAIL", L.Label, L.Got.c_str());
        if (!L.Pass) {
            ++Failed;
        }
    }

    const auto Elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - Started)
                             .count();
    std::printf("\n%s — %.1fs\n", Failed == 0 ? "DEPTH CONFIRMED" : "THRESHOLD(S) FAILED",
                static_cast<double>(Elapsed) / 1000.0);
    return Failed == 0 ? 0 : 1;
}
