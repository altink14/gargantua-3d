#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace Gargantua {

/**
 * The ten elements, in wheel order.
 *
 * An enum rather than the strings the original browser build used. The search
 * AI compares elements millions of times per match, and this turns each of
 * those comparisons into an integer compare and each table lookup into an
 * array index. It is also impossible to misspell one.
 *
 * Order is load-bearing. Reordering this rebalances the entire game.
 */
enum class ElementId : std::uint8_t {
    Ember = 0,
    Bloom,
    Venom,
    Stone,
    Frost,
    Spark,
    Gale,
    Tide,
    Lumen,
    Umbra,
    Count,
};

inline constexpr int ElementCount = static_cast<int>(ElementId::Count);

/**
 * The wheel.
 *
 * Each element is strong against the next two and weak against the previous
 * two. Ten rules instead of a hundred, which is the difference between a wheel
 * a player learns and a table they look up.
 *
 * A shot is scored against what the DEFENDER last fired, so the slug you pick
 * also decides what you are exposed to next turn. At 1.5x that tradeoff was too
 * small to be worth thinking about; at 2x, firing your biggest attack can
 * genuinely be the wrong move.
 */
inline constexpr float StrongMultiplier = 2.0f;
inline constexpr float WeakMultiplier = 0.5f;
inline constexpr float NeutralMultiplier = 1.0f;

struct ElementInfo {
    ElementId Id;
    std::string_view Name;
    /** Hex, used for glow and creature tinting. Chosen to stay distinct in the dark. */
    std::string_view Color;
    /** Shown in the codex beside the wheel, so matchups read as reasons not rules. */
    std::string_view Note;
};

inline constexpr std::array<ElementInfo, ElementCount> Elements{{
    {ElementId::Ember, "Ember", "#ff6b35", "Burns what grows and cooks what festers."},
    {ElementId::Bloom, "Bloom", "#6ee7a0", "Roots drink poison and split stone."},
    {ElementId::Venom, "Venom", "#a855f7", "Eats through rock and will not freeze."},
    {ElementId::Stone, "Stone", "#b08968", "Shrugs off ice and swallows current."},
    {ElementId::Frost, "Frost", "#7dd3fc", "Dulls a charge and stills the wind."},
    {ElementId::Spark, "Spark", "#fbbf24", "Rides the wind and owns the water."},
    {ElementId::Gale, "Gale", "#cbd5e1", "Whips the tide and scatters light to dust."},
    {ElementId::Tide, "Tide", "#38bdf8", "Drowns a glare and washes shadow away."},
    {ElementId::Lumen, "Lumen", "#fef08a", "Banishes the dark and outshines any flame."},
    {ElementId::Umbra, "Umbra", "#8b5cf6", "Smothers fire and starves anything green."},
}};

inline constexpr const ElementInfo& ElementOf(ElementId Id) {
    return Elements[static_cast<int>(Id)];
}

/**
 * Damage multiplier for `Attacker` hitting `Defender`.
 *
 * Total and deterministic: every pair returns exactly one of the three
 * multipliers, and Matchup(a, b) being strong implies Matchup(b, a) is weak.
 * The tests assert both, over all one hundred pairs.
 */
inline constexpr float Matchup(ElementId Attacker, ElementId Defender) {
    const int N = ElementCount;
    const int Step = (static_cast<int>(Defender) - static_cast<int>(Attacker) + N) % N;
    if (Step == 1 || Step == 2) {
        return StrongMultiplier;
    }
    if (Step == N - 1 || Step == N - 2) {
        return WeakMultiplier;
    }
    return NeutralMultiplier;
}

/** The two elements this one beats. Used by the codex wheel. */
inline constexpr std::array<ElementId, 2> StrongAgainst(ElementId Id) {
    const int N = ElementCount;
    const int I = static_cast<int>(Id);
    return {static_cast<ElementId>((I + 1) % N), static_cast<ElementId>((I + 2) % N)};
}

/** The two elements that beat this one. Used by the codex wheel. */
inline constexpr std::array<ElementId, 2> WeakAgainst(ElementId Id) {
    const int N = ElementCount;
    const int I = static_cast<int>(Id);
    return {static_cast<ElementId>((I - 1 + N) % N), static_cast<ElementId>((I - 2 + N) % N)};
}

}  // namespace Gargantua
