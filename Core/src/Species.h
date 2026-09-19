#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "Elements.h"
#include "Types.h"

namespace Gargantua {

/**
 * The eighteen slugs.
 *
 * An enum for the engine and a stable string for the save file. The enum makes
 * the search fast and misspelling impossible; the string is what persists, so
 * reordering this list can never silently turn somebody's Cinderling into a
 * Hollowfang.
 *
 * Power and cooldown are the only balance levers. Higher power always costs a
 * longer cooldown — a slug that hits hard and recovers fast has no reason to
 * ever stay in its case.
 */
enum class SpeciesId : std::uint8_t {
    Cinderling = 0,
    Pyrelash,
    Thornwhistle,
    Bramblehusk,
    Mirebubble,
    Gritmaw,
    Rimeclutch,
    Voltmote,
    Skirlwing,
    Dewspine,
    Glimmerquill,
    Nullmoth,
    Slagmaw,
    Wispwick,
    Hollowfang,
    Brinemoth,
    Gallowspark,
    Chillblain,
    Count,
    None = 0xFF,
};

inline constexpr int SpeciesCount = static_cast<int>(SpeciesId::Count);

struct Species {
    SpeciesId Id;
    /** Stable kebab-case identity. Save files key on this, so it never changes. */
    std::string_view Key;
    std::string_view Name;
    ElementId Element;
    RoleId Role;
    Rarity RarityTier;
    /** Base magnitude, 1..10. Scaled by element matchup, never by chance. */
    int Power;
    /** Turns this slug is unavailable after firing, 1..4. */
    int Cooldown;
    /** One sentence, shown in the codex. Written to be read, not skimmed. */
    std::string_view Flavor;
};

inline constexpr std::array<Species, SpeciesCount> AllSpecies{{
    {SpeciesId::Cinderling, "cinderling", "Cinderling", ElementId::Ember, RoleId::Burner, Rarity::Common, 4, 1,
     "Sheds sparks constantly and seems faintly embarrassed about it."},
    {SpeciesId::Pyrelash, "pyrelash", "Pyrelash", ElementId::Ember, RoleId::Exploder, Rarity::Rare, 8, 3,
     "Holds its breath for three days, then spends it all at once."},
    {SpeciesId::Thornwhistle, "thornwhistle", "Thornwhistle", ElementId::Bloom, RoleId::Healer, Rarity::Common, 3, 2,
     "Hums through hollow spines; the sound knits what it passes over."},
    {SpeciesId::Bramblehusk, "bramblehusk", "Bramblehusk", ElementId::Bloom, RoleId::Vanguard, Rarity::Uncommon, 4, 2,
     "Grows a thicket faster than most things can decide to run."},
    {SpeciesId::Mirebubble, "mirebubble", "Mirebubble", ElementId::Venom, RoleId::Burner, Rarity::Common, 4, 2,
     "Bursts into a cloud that lingers long after everyone has moved on."},
    {SpeciesId::Gritmaw, "gritmaw", "Gritmaw", ElementId::Stone, RoleId::Digger, Rarity::Common, 5, 2,
     "Goes under. Always under. Walls are a suggestion."},
    {SpeciesId::Rimeclutch, "rimeclutch", "Rimeclutch", ElementId::Frost, RoleId::Trickster, Rarity::Uncommon, 4, 3,
     "Freezes a slug mid-recovery, which they take personally."},
    {SpeciesId::Voltmote, "voltmote", "Voltmote", ElementId::Spark, RoleId::Exploder, Rarity::Common, 6, 3,
     "Small, bright, and entirely out of proportion to its size."},
    {SpeciesId::Skirlwing, "skirlwing", "Skirlwing", ElementId::Gale, RoleId::Trickster, Rarity::Common, 4, 2,
     "Rides its own draft in tight circles to stall anything behind it."},
    {SpeciesId::Dewspine, "dewspine", "Dewspine", ElementId::Tide, RoleId::Healer, Rarity::Uncommon, 3, 2,
     "Carries a full cavern pool somewhere it should not fit."},
    {SpeciesId::Glimmerquill, "glimmerquill", "Glimmerquill", ElementId::Lumen, RoleId::Digger, Rarity::Rare, 6, 3,
     "Arrives before its own light does, which is how it gets past guards."},
    {SpeciesId::Nullmoth, "nullmoth", "Nullmoth", ElementId::Umbra, RoleId::Vanguard, Rarity::Rare, 5, 3,
     "Folds the dark into a shape that holds. Nobody has asked it how."},
    {SpeciesId::Slagmaw, "slagmaw", "Slagmaw", ElementId::Stone, RoleId::Exploder, Rarity::Uncommon, 7, 3,
     "Swallows a mouthful of molten rock and holds it until the timing suits."},
    {SpeciesId::Wispwick, "wispwick", "Wispwick", ElementId::Lumen, RoleId::Healer, Rarity::Uncommon, 4, 2,
     "Burns steadily at one end. Whatever it lights up starts mending."},
    {SpeciesId::Hollowfang, "hollowfang", "Hollowfang", ElementId::Umbra, RoleId::Digger, Rarity::Rare, 7, 3,
     "Bites a hole in the dark and steps through it a few feet further on."},
    {SpeciesId::Brinemoth, "brinemoth", "Brinemoth", ElementId::Tide, RoleId::Vanguard, Rarity::Uncommon, 5, 2,
     "Throws up a wall of standing water that refuses to fall over."},
    {SpeciesId::Gallowspark, "gallowspark", "Gallowspark", ElementId::Spark, RoleId::Trickster, Rarity::Rare, 5, 3,
     "Earths itself through whatever is recovering and keeps it there."},
    {SpeciesId::Chillblain, "chillblain", "Chillblain", ElementId::Frost, RoleId::Burner, Rarity::Uncommon, 5, 2,
     "The cold it leaves behind keeps biting long after the shot lands."},
}};

inline constexpr const Species& SpeciesOf(SpeciesId Id) {
    return AllSpecies[static_cast<int>(Id)];
}

/** Looks a slug up by its saved key. Returns None for anything unrecognised. */
inline constexpr SpeciesId SpeciesFromKey(std::string_view Key) {
    for (const Species& S : AllSpecies) {
        if (S.Key == Key) {
            return S.Id;
        }
    }
    return SpeciesId::None;
}

/**
 * The pair every new slinger starts with.
 *
 * Deliberately a pair, and deliberately one that fuses: Cinderling and
 * Skirlwing make Firestorm. A player who starts with a single slug cannot fuse
 * anything, which means the mechanic the whole game rests on would be
 * unreachable in their first duel.
 */
inline constexpr std::array<SpeciesId, 2> StarterSpecies{
    SpeciesId::Cinderling,
    SpeciesId::Skirlwing,
};

}  // namespace Gargantua
