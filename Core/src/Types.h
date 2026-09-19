#pragma once

#include <cstdint>
#include <string_view>

namespace Gargantua {

/**
 * What a slug *does* when it lands.
 *
 * Element decides how hard it hits; role decides what shape the hit takes. Two
 * slugs of the same element and different roles should never feel
 * interchangeable, and in the 3D build the role also decides the silhouette.
 */
enum class RoleId : std::uint8_t {
    Vanguard = 0,  // grants shield instead of dealing damage
    Burner,        // light hit now, more over the following turns
    Digger,        // ignores shield entirely
    Exploder,      // heavy hit, long cooldown
    Healer,        // restores health, deals nothing
    Trickster,     // extends an opponent slug's cooldown
    Count,
};

enum class Rarity : std::uint8_t {
    Common = 0,
    Uncommon,
    Rare,
    Ultra,
    Count,
};

inline constexpr std::string_view RoleName(RoleId Role) {
    switch (Role) {
        case RoleId::Vanguard: return "Vanguard";
        case RoleId::Burner: return "Burner";
        case RoleId::Digger: return "Digger";
        case RoleId::Exploder: return "Exploder";
        case RoleId::Healer: return "Healer";
        case RoleId::Trickster: return "Trickster";
        default: return "Unknown";
    }
}

/** How many slugs you may bring when duelling a wild slug of this rarity. */
inline constexpr int LoadoutCapFor(Rarity R) {
    switch (R) {
        case Rarity::Common: return 3;
        case Rarity::Uncommon: return 3;
        case Rarity::Rare: return 2;
        case Rarity::Ultra: return 1;
        default: return 3;
    }
}

/** Which slugs a stall reaches. */
enum class StallScope : std::uint8_t {
    None = 0,
    Longest,  // only their slowest
    All,      // every one recovering
};

/**
 * What a shot actually does when it resolves.
 *
 * A species' effect is derived from its role; a fusion states its own, because
 * the point of a fusion is to do something neither parent could. Every field is
 * additive — a shot with nothing set simply deals its damage.
 */
struct EffectSpec {
    /** Damage passes through shield untouched. */
    bool IgnoresShield = false;
    /** Damage repeats at the start of the target's turn for this many turns. */
    int BurnTurns = 0;
    /** Restores this much health to the firing side instead of dealing damage. */
    int Heal = 0;
    /** Grants this much shield to the firing side instead of dealing damage. */
    int Shield = 0;
    int StallTurns = 0;
    StallScope Stall = StallScope::None;

    /** Support shots deal no damage, so the element wheel never applies to them. */
    constexpr bool IsSupport() const { return Heal > 0 || Shield > 0; }
};

/**
 * The role-derived effect for a species. Magnitudes scale off its power.
 *
 * Heal and shield are deliberately modest. When they were larger, a quarter of
 * all matches never resolved: sustain simply outran damage and both sides
 * ground on forever. Raise these only with a harness run to back it up.
 */
inline constexpr EffectSpec EffectForRole(RoleId Role, int Power) {
    EffectSpec E{};
    switch (Role) {
        case RoleId::Vanguard: E.Shield = Power; break;
        case RoleId::Healer: E.Heal = Power + 1; break;
        case RoleId::Burner: E.BurnTurns = 2; break;
        case RoleId::Digger: E.IgnoresShield = true; break;
        case RoleId::Trickster:
            E.StallTurns = 1;
            E.Stall = StallScope::Longest;
            break;
        case RoleId::Exploder: break;
        default: break;
    }
    return E;
}

}  // namespace Gargantua
