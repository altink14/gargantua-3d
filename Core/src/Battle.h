#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "Elements.h"
#include "Fusions.h"
#include "Species.h"
#include "Types.h"

namespace Gargantua {

/**
 * The battle engine.
 *
 * One rule governs this file: ApplyAction is pure and deterministic. The same
 * state plus the same action always produces the same next state, with no
 * clock, no randomness, and no mutation of the input. That is what makes the
 * game chess rather than dice, makes every match replayable from its action
 * list, and is why the depth harness can trust its own numbers.
 *
 * It also never fails loudly. An illegal action returns a state with an Illegal
 * event recorded, so the interface always has something specific to tell the
 * player rather than an exception to swallow.
 *
 * Differences from the browser build, both deliberate:
 *  - No heap. A side's roster, burns and the whole state are fixed-size, so the
 *    search can copy a position by value and never allocate.
 *  - The log is optional. Search passes nullptr and pays nothing for events
 *    nobody reads; the game passes a log and gets the full narration.
 */

inline constexpr int MaxLoadout = 5;
inline constexpr int BaseHp = 40;

/** Fraction of a shot's damage that a burn repeats each turn. */
inline constexpr float BurnFraction = 0.3f;
/** Shield bleeds away each turn, so it is a question of timing, not a deposit. */
inline constexpr int ShieldDecay = 2;

/**
 * Ceilings on the values that could otherwise grow without limit.
 *
 * Shield reached 899 in an adversarial run of the browser build, because a pair
 * of support slugs can add more per turn than the decay removes. A stall loop
 * pushed a cooldown to 153, locking a slug out of the match permanently. And a
 * duel between two damageless teams never ended at all.
 */
inline constexpr int MaxShield = 20;
inline constexpr int MaxCooldown = 6;
/**
 * Turns before a duel nobody can finish is called a draw.
 *
 * A safety net for a match that genuinely cannot end, not a game rule. At 60 it
 * robbed people: a duel drew with the opponent on 2 health that would have been
 * won two turns later. Every shipped encounter resolves by turn 10 and the
 * longest decidable match found was 115.
 */
inline constexpr int MaxTurns = 200;

/** Burns stack. Eight at once is far past anything reachable in a real duel. */
inline constexpr int MaxBurns = 8;

struct BattleCreature {
    SpeciesId Id = SpeciesId::None;
    int CooldownRemaining = 0;
};

struct BurnStatus {
    int TurnsRemaining = 0;
    int DamagePerTurn = 0;
};

struct SideState {
    int Hp = BaseHp;
    int MaxHp = BaseHp;
    int Shield = 0;
    std::array<BattleCreature, MaxLoadout> Roster{};
    int RosterCount = 0;
    /**
     * The last slug this side fired. Does double duty: it decides which fusion
     * is reachable next turn, and it is the element an incoming shot is scored
     * against. Both are visible to the opponent — this game has no hidden state.
     */
    SpeciesId LastFired = SpeciesId::None;
    std::array<BurnStatus, MaxBurns> Burns{};
    int BurnCount = 0;
};

enum class ActionType : std::uint8_t { Fire, Hold, Forfeit };

struct Action {
    ActionType Type = ActionType::Hold;
    SpeciesId Species = SpeciesId::None;
};

enum class EventType : std::uint8_t {
    Fire,
    BurnTick,
    Heal,
    Shield,
    ShieldFade,
    Stall,
    Hold,
    Illegal,
    Forfeit,
    Win,
    Draw,
};

struct BattleEvent {
    EventType Type = EventType::Hold;
    int Side = 0;
    SpeciesId Species = SpeciesId::None;
    /** The fusion's name when Fused, otherwise the species' name. */
    std::string_view ShotName;
    ElementId Element = ElementId::Ember;
    int Damage = 0;
    int Blocked = 0;
    int Amount = 0;
    float Multiplier = 1.0f;
    bool Fused = false;
    bool Support = false;
    std::string_view Reason;
};

using BattleLog = std::vector<BattleEvent>;

struct BattleState {
    int Turn = 0;
    int Active = 0;
    std::array<SideState, 2> Sides{};
    /** -1 while nobody has won. */
    int Winner = -1;
    /**
     * Nobody could finish it. Two sides made entirely of healers and vanguards
     * deal no damage at all, so without this the duel runs forever and the only
     * way out is closing the game.
     */
    bool Drawn = false;

    bool IsOver() const { return Winner >= 0 || Drawn; }
};

inline SideState MakeSide(const SpeciesId* Ids, int Count, int Hp = BaseHp) {
    SideState S{};
    S.Hp = Hp;
    S.MaxHp = Hp;
    S.RosterCount = Count < MaxLoadout ? Count : MaxLoadout;
    for (int I = 0; I < S.RosterCount; ++I) {
        S.Roster[I] = BattleCreature{Ids[I], 0};
    }
    return S;
}

inline BattleState MakeBattle(const SideState& A, const SideState& B) {
    BattleState State{};
    State.Sides[0] = A;
    State.Sides[1] = B;
    return State;
}

inline constexpr int Other(int Side) { return Side == 0 ? 1 : 0; }

struct Shot {
    std::string_view Name;
    ElementId Element;
    int Power;
    EffectSpec Effect;
    bool Fused;
};

/** What firing Id would resolve as, given what this side fired last. */
inline Shot ResolveShot(const BattleState& State, int Side, SpeciesId Id) {
    if (const Fusion* F = FindFusion(State.Sides[Side].LastFired, Id)) {
        return Shot{F->Name, F->Element, F->Power, F->Effect, true};
    }
    const Species& S = SpeciesOf(Id);
    return Shot{S.Name, S.Element, S.Power, EffectForRole(S.Role, S.Power), false};
}

/**
 * Damage multiplier for a shot, scored against what the defender last fired.
 * Before they have fired anything there is nothing to counter, so it is even.
 */
inline float ShotMultiplier(const BattleState& State, int Side, ElementId Element) {
    const SpeciesId DefenderLast = State.Sides[Other(Side)].LastFired;
    if (DefenderLast == SpeciesId::None) {
        return NeutralMultiplier;
    }
    return Matchup(Element, SpeciesOf(DefenderLast).Element);
}

/** Whether this side may fire that slug right now. */
inline bool IsReady(const SideState& Side, SpeciesId Id) {
    for (int I = 0; I < Side.RosterCount; ++I) {
        if (Side.Roster[I].Id == Id) {
            return Side.Roster[I].CooldownRemaining == 0;
        }
    }
    return false;
}

namespace Detail {

inline void Emit(BattleLog* Log, const BattleEvent& E) {
    if (Log != nullptr) {
        Log->push_back(E);
    }
}

/** Start-of-turn upkeep for the side about to act: burns tick, shield fades,
 *  cooldowns fall. */
inline void BeginTurn(BattleState& State, int Side, BattleLog* Log) {
    SideState& S = State.Sides[Side];

    int Kept = 0;
    for (int I = 0; I < S.BurnCount; ++I) {
        BurnStatus& B = S.Burns[I];
        S.Hp -= B.DamagePerTurn;
        Emit(Log, BattleEvent{EventType::BurnTick, Side, SpeciesId::None, {}, ElementId::Ember,
                              B.DamagePerTurn});
        B.TurnsRemaining -= 1;
        if (B.TurnsRemaining > 0) {
            S.Burns[Kept++] = B;
        }
    }
    S.BurnCount = Kept;

    if (S.Shield > 0) {
        const int Lost = S.Shield < ShieldDecay ? S.Shield : ShieldDecay;
        S.Shield -= Lost;
        BattleEvent E{EventType::ShieldFade, Side};
        E.Amount = Lost;
        Emit(Log, E);
    }

    for (int I = 0; I < S.RosterCount; ++I) {
        if (S.Roster[I].CooldownRemaining > 0) {
            S.Roster[I].CooldownRemaining -= 1;
        }
    }

    if (S.Hp <= 0) {
        S.Hp = 0;
        State.Winner = Other(Side);
        Emit(Log, BattleEvent{EventType::Win, Other(Side)});
    }
}

inline void EndTurn(BattleState& State, BattleLog* Log) {
    if (State.Winner >= 0) {
        return;
    }
    State.Active = Other(State.Active);
    State.Turn += 1;

    if (State.Turn >= MaxTurns) {
        State.Drawn = true;
        Emit(Log, BattleEvent{EventType::Draw});
        return;
    }

    BeginTurn(State, State.Active, Log);
}

inline void ApplySelfEffect(SideState& Side, const EffectSpec& Effect, int Idx, BattleLog* Log) {
    if (Effect.Heal > 0) {
        const int Before = Side.Hp;
        Side.Hp = Side.Hp + Effect.Heal > Side.MaxHp ? Side.MaxHp : Side.Hp + Effect.Heal;
        BattleEvent E{EventType::Heal, Idx};
        E.Amount = Side.Hp - Before;
        Emit(Log, E);
    }
    if (Effect.Shield > 0) {
        const int Before = Side.Shield;
        const int Raised = Side.Shield + Effect.Shield;
        Side.Shield = Raised > MaxShield ? MaxShield : Raised;
        const int Gained = Side.Shield - Before;
        if (Gained > 0) {
            BattleEvent E{EventType::Shield, Idx};
            E.Amount = Gained;
            Emit(Log, E);
        }
    }
}

inline void ApplyStall(SideState& Target, const EffectSpec& Effect, int Idx, BattleLog* Log) {
    if (Effect.Stall == StallScope::None || Effect.StallTurns <= 0) {
        return;
    }

    int Longest = -1;
    for (int I = 0; I < Target.RosterCount; ++I) {
        if (Target.Roster[I].CooldownRemaining <= 0) {
            continue;
        }
        if (Effect.Stall == StallScope::All) {
            Target.Roster[I].CooldownRemaining =
                std::min(MaxCooldown, Target.Roster[I].CooldownRemaining + Effect.StallTurns);
            BattleEvent E{EventType::Stall, Idx, Target.Roster[I].Id};
            E.Amount = Effect.StallTurns;
            Emit(Log, E);
        } else if (Longest < 0 ||
                   Target.Roster[I].CooldownRemaining > Target.Roster[Longest].CooldownRemaining) {
            Longest = I;
        }
    }

    if (Effect.Stall == StallScope::Longest && Longest >= 0) {
        Target.Roster[Longest].CooldownRemaining =
            std::min(MaxCooldown, Target.Roster[Longest].CooldownRemaining + Effect.StallTurns);
        BattleEvent E{EventType::Stall, Idx, Target.Roster[Longest].Id};
        E.Amount = Effect.StallTurns;
        Emit(Log, E);
    }
}

}  // namespace Detail

/**
 * The contract. Pure: never mutates State, never throws, always returns a
 * usable position. An illegal action costs the player nothing but an
 * explanation.
 */
inline BattleState ApplyAction(const BattleState& State, const Action& Act, BattleLog* Log = nullptr) {
    BattleState Next = State;

    // A finished battle ignores further actions, but still hands back a copy:
    // returning the caller's own object invites editing history in place.
    if (State.IsOver()) {
        return Next;
    }

    const int Side = Next.Active;
    const int ThemIdx = Other(Side);
    SideState& Me = Next.Sides[Side];
    SideState& Them = Next.Sides[ThemIdx];

    if (Act.Type == ActionType::Forfeit) {
        Detail::Emit(Log, BattleEvent{EventType::Forfeit, Side});
        Next.Winner = ThemIdx;
        Detail::Emit(Log, BattleEvent{EventType::Win, ThemIdx});
        return Next;
    }

    if (Act.Type == ActionType::Hold) {
        // Holding drops your stance. You stop being exposed to whatever counters
        // the slug you last fired, but you also give up the fusion it was setting
        // up. That trade is the whole reason this move exists.
        Me.LastFired = SpeciesId::None;
        Detail::Emit(Log, BattleEvent{EventType::Hold, Side});
        Detail::EndTurn(Next, Log);
        return Next;
    }

    // Checked before anything reads the species table, so an id the content does
    // not know about is an explanation rather than a crash.
    if (Act.Species == SpeciesId::None || static_cast<int>(Act.Species) >= SpeciesCount) {
        BattleEvent E{EventType::Illegal, Side};
        E.Reason = "That slug is not one this world knows about.";
        Detail::Emit(Log, E);
        return Next;
    }

    int Slot = -1;
    for (int I = 0; I < Me.RosterCount; ++I) {
        if (Me.Roster[I].Id == Act.Species) {
            Slot = I;
            break;
        }
    }
    if (Slot < 0) {
        BattleEvent E{EventType::Illegal, Side};
        E.Reason = "That slug is not in your loadout.";
        Detail::Emit(Log, E);
        return Next;
    }
    if (Me.Roster[Slot].CooldownRemaining > 0) {
        BattleEvent E{EventType::Illegal, Side, Act.Species};
        E.Amount = Me.Roster[Slot].CooldownRemaining;
        E.Reason = "That slug is still recovering.";
        Detail::Emit(Log, E);
        return Next;
    }

    const Shot S = ResolveShot(Next, Side, Act.Species);
    const bool Support = S.Effect.IsSupport();
    // A support shot deals no damage, so the element wheel never applies to it.
    // Reporting a multiplier here would teach a rule that does not exist.
    const float Multiplier = Support ? NeutralMultiplier : ShotMultiplier(Next, Side, S.Element);

    int Damage = 0;
    int Blocked = 0;

    if (!Support) {
        Damage = static_cast<int>(std::lround(static_cast<float>(S.Power) * Multiplier));
        if (!S.Effect.IgnoresShield && Them.Shield > 0) {
            Blocked = Them.Shield < Damage ? Them.Shield : Damage;
            Them.Shield -= Blocked;
            Damage -= Blocked;
        }
        Them.Hp -= Damage;

        if (S.Effect.BurnTurns > 0 && Them.BurnCount < MaxBurns) {
            const int PerTurn = static_cast<int>(
                std::lround(static_cast<float>(S.Power) * Multiplier * BurnFraction));
            Them.Burns[Them.BurnCount++] =
                BurnStatus{S.Effect.BurnTurns, PerTurn < 1 ? 1 : PerTurn};
        }
    }

    BattleEvent Fired{EventType::Fire, Side, Act.Species, S.Name, S.Element, Damage, Blocked};
    Fired.Multiplier = Multiplier;
    Fired.Fused = S.Fused;
    Fired.Support = Support;
    Detail::Emit(Log, Fired);

    Detail::ApplySelfEffect(Me, S.Effect, Side, Log);
    Detail::ApplyStall(Them, S.Effect, ThemIdx, Log);

    Me.Roster[Slot].CooldownRemaining = SpeciesOf(Act.Species).Cooldown;
    Me.LastFired = Act.Species;

    if (Them.Hp <= 0) {
        Them.Hp = 0;
        Next.Winner = Side;
        Detail::Emit(Log, BattleEvent{EventType::Win, Side});
        return Next;
    }

    Detail::EndTurn(Next, Log);
    return Next;
}

}  // namespace Gargantua
