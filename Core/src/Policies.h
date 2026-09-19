#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>

#include "Battle.h"

namespace Gargantua {

/**
 * Three ways to play, so the game's depth can be measured instead of asserted.
 *
 * The claim under test is that Gargantua rewards thinking ahead. That is only a
 * claim until a player who plans beats a player who grabs the biggest number,
 * and that player beats one choosing at random. These are those three players.
 *
 * Every policy here is deterministic given its inputs — Random takes an
 * explicit seeded generator rather than a global one — so any result the
 * harness reports can be reproduced exactly.
 */

/** Small seeded PRNG. Reproducible runs matter more than statistical purity. */
class Rng {
public:
    explicit Rng(std::uint32_t Seed) : State(Seed) {}

    std::uint32_t Next() {
        State += 0x6d2b79f5u;
        std::uint32_t T = State;
        T = (T ^ (T >> 15)) * (T | 1u);
        T ^= T + (T ^ (T >> 7)) * (T | 61u);
        return T ^ (T >> 14);
    }

    int Below(int Bound) { return Bound <= 0 ? 0 : static_cast<int>(Next() % static_cast<std::uint32_t>(Bound)); }

private:
    std::uint32_t State;
};

/** Every action the engine will actually accept right now. Never Forfeit. */
inline int LegalActions(const BattleState& State, int Side, std::array<Action, MaxLoadout + 1>& Out) {
    int Count = 0;
    const SideState& S = State.Sides[Side];
    for (int I = 0; I < S.RosterCount; ++I) {
        if (S.Roster[I].CooldownRemaining == 0) {
            Out[Count++] = Action{ActionType::Fire, S.Roster[I].Id};
        }
    }
    Out[Count++] = Action{ActionType::Hold};
    return Count;
}

inline int BurnPending(const SideState& S) {
    int Total = 0;
    for (int I = 0; I < S.BurnCount; ++I) {
        Total += S.Burns[I].DamagePerTurn * S.Burns[I].TurnsRemaining;
    }
    return Total;
}

inline int ReadyCount(const SideState& S) {
    int Count = 0;
    for (int I = 0; I < S.RosterCount; ++I) {
        if (S.Roster[I].CooldownRemaining == 0) {
            ++Count;
        }
    }
    return Count;
}

/**
 * How good this position is for Side, in health-equivalents.
 *
 * Shield counts as health because that is what it does. Pending burn is counted
 * now rather than when it lands, so the search does not treat a four-turn burn
 * as free. Having slugs ready is worth something on its own — a full blaster
 * and no options is how you lose.
 */
inline float Evaluate(const BattleState& State, int Side) {
    const int Foe = Other(Side);
    const SideState& Me = State.Sides[Side];
    const SideState& Them = State.Sides[Foe];

    float Score = static_cast<float>((Me.Hp + Me.Shield - BurnPending(Me)) -
                                     (Them.Hp + Them.Shield - BurnPending(Them)));

    Score += static_cast<float>(ReadyCount(Me)) * 1.5f;
    Score -= static_cast<float>(ReadyCount(Them)) * 1.5f;

    // A fusion that is reachable next turn is worth steering toward.
    if (Me.LastFired != SpeciesId::None) {
        for (int I = 0; I < Me.RosterCount; ++I) {
            if (Me.Roster[I].CooldownRemaining == 0 && FindFusion(Me.LastFired, Me.Roster[I].Id)) {
                Score += 4.0f;
                break;
            }
        }
    }
    return Score;
}

inline constexpr float WinScore = 1.0e6f;

/**
 * Negamax with alpha-beta over the real engine.
 *
 * The engine is pure and deterministic, so search can simply play moves and
 * look at what comes back — there is no separate rules model to drift out of
 * sync with the game. Nothing is logged during search, so the cost of an
 * unread event is never paid.
 */
inline float Search(const BattleState& State, int Depth, float Alpha, float Beta, int RootSide) {
    if (State.Winner >= 0) {
        return State.Winner == RootSide ? WinScore + static_cast<float>(Depth)
                                        : -WinScore - static_cast<float>(Depth);
    }
    if (State.Drawn || Depth == 0) {
        return Evaluate(State, RootSide);
    }

    const bool Maximizing = State.Active == RootSide;
    float Best = Maximizing ? -std::numeric_limits<float>::infinity()
                            : std::numeric_limits<float>::infinity();

    std::array<Action, MaxLoadout + 1> Actions{};
    const int Count = LegalActions(State, State.Active, Actions);

    for (int I = 0; I < Count; ++I) {
        const BattleState Next = ApplyAction(State, Actions[I]);
        const float Value = Search(Next, Depth - 1, Alpha, Beta, RootSide);

        if (Maximizing) {
            Best = std::max(Best, Value);
            Alpha = std::max(Alpha, Best);
        } else {
            Best = std::min(Best, Value);
            Beta = std::min(Beta, Best);
        }
        if (Beta <= Alpha) {
            break;
        }
    }
    return Best;
}

/**
 * Clamps a caller-supplied depth to something the search can actually finish.
 *
 * Search stops on Depth == 0, so a depth of zero or a negative never lands on
 * the base case and recurses until the process dies. Trainer depth is authored
 * content, which means a typo in a data file could hang the game with no error
 * at all.
 */
inline int UsableDepth(int Depth) { return std::clamp(Depth, 1, 12); }

/** The floor. If this competes with the others, the game is noise. */
inline Action RandomPolicy(const BattleState& State, int Side, Rng& Generator) {
    std::array<Action, MaxLoadout + 1> Actions{};
    const int Count = LegalActions(State, Side, Actions);
    return Actions[Generator.Below(Count)];
}

/** Fusions outrank everything else that could be fired this turn. */
inline constexpr float FusionBonus = 1000.0f;
/** A strong matchup outranks any amount of raw power at these numbers. */
inline constexpr float StrongBonus = 100.0f;

/**
 * Scores only the shot in front of it: a fusion beats everything, a strong
 * matchup beats a neutral one, and otherwise the biggest number wins. It does
 * not plan a sequence, bait a cooldown, or hold to set up a fusion next turn.
 *
 * Not the opponent the game ships, but the harness's middle baseline. "Thinking
 * ahead beats grabbing the biggest number" is only a measurable claim while
 * something still grabs the biggest number.
 */
inline Action GreedyPolicy(const BattleState& State, int Side) {
    if (State.IsOver()) {
        return Action{ActionType::Hold};
    }

    const SideState& Me = State.Sides[Side];
    SpeciesId BestId = SpeciesId::None;
    float BestScore = -std::numeric_limits<float>::infinity();

    for (int I = 0; I < Me.RosterCount; ++I) {
        if (Me.Roster[I].CooldownRemaining != 0) {
            continue;
        }
        const SpeciesId Id = Me.Roster[I].Id;
        const Shot S = ResolveShot(State, Side, Id);
        const bool Support = S.Effect.IsSupport();
        const float Multiplier = Support ? NeutralMultiplier : ShotMultiplier(State, Side, S.Element);

        // Healing at full health is worth nothing, so support is valued by what
        // it would actually gain rather than by the numbers printed on the slug.
        float Value;
        if (Support) {
            const int Gained = std::min(S.Effect.Heal, Me.MaxHp - Me.Hp);
            Value = static_cast<float>(Gained + S.Effect.Shield);
        } else {
            Value = static_cast<float>(S.Power) * Multiplier;
        }

        float Score = Value;
        if (S.Fused) {
            Score += FusionBonus;
        }
        if (!Support && Multiplier > NeutralMultiplier) {
            Score += StrongBonus;
        }

        // Strictly greater keeps ties resolving to roster order, so the same
        // position always produces the same move and a match stays replayable.
        if (Score > BestScore) {
            BestScore = Score;
            BestId = Id;
        }
    }

    return BestId == SpeciesId::None ? Action{ActionType::Hold} : Action{ActionType::Fire, BestId};
}

/** The player who thinks ahead. */
inline Action TacticalPolicy(const BattleState& State, int Side, int Depth) {
    // A finished battle has no move to find.
    if (State.IsOver()) {
        return Action{ActionType::Hold};
    }

    const int D = UsableDepth(Depth);
    std::array<Action, MaxLoadout + 1> Actions{};
    const int Count = LegalActions(State, Side, Actions);

    Action Best = Actions[0];
    float BestScore = -std::numeric_limits<float>::infinity();

    for (int I = 0; I < Count; ++I) {
        const BattleState Next = ApplyAction(State, Actions[I]);
        const float Score = Search(Next, D - 1, -std::numeric_limits<float>::infinity(),
                                   std::numeric_limits<float>::infinity(), Side);
        if (Score > BestScore) {
            BestScore = Score;
            Best = Actions[I];
        }
    }
    return Best;
}

}  // namespace Gargantua
