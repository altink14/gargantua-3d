#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "Elements.h"
#include "Species.h"
#include "Types.h"

namespace Gargantua {

/**
 * Fusions are the whole game.
 *
 * Fire two slugs on consecutive turns and, if they pair, the second shot
 * resolves as the fusion instead of as itself. That single rule is what turns a
 * loadout into a sequencing problem: cooldowns force you to rotate, rotation
 * decides which fusions you can still reach, and your opponent can see all of
 * it. Elements alone would be a stat check. This is not.
 *
 * Density is load-bearing, and was the reason the first version of this game
 * failed its depth test. With only eight fusions, two thirds of loadouts could
 * reach at most one, so there was never a choice about which to set up — only
 * whether you could. Every species now takes part in three or four.
 *
 * This table was generated from the browser build whose balance is proven, so
 * the two cannot drift apart. Verify any change with the depth harness.
 */
struct Fusion {
    SpeciesId A;
    SpeciesId B;
    std::string_view Name;
    int Power;
    ElementId Element;
    EffectSpec Effect;
    /** One sentence a player can act on: what it does, not how it feels. */
    std::string_view Description;
};

inline constexpr std::array<Fusion, 44> AllFusions{{
    {SpeciesId::Cinderling, SpeciesId::Voltmote, "Plasmalash", 11,
     ElementId::Spark, EffectSpec{.BurnTurns = 2},
     "Heavy hit that also burns for two turns."},
    {SpeciesId::Cinderling, SpeciesId::Skirlwing, "Firestorm", 7,
     ElementId::Ember, EffectSpec{.BurnTurns = 4, .StallTurns = 1, .Stall = StallScope::Longest},
     "Burns for four turns instead of two, and stalls a slug in recovery."},
    {SpeciesId::Gritmaw, SpeciesId::Mirebubble, "Acidbore", 8,
     ElementId::Venom, EffectSpec{.IgnoresShield = true, .BurnTurns = 3},
     "Ignores shield and keeps burning after it lands."},
    {SpeciesId::Bramblehusk, SpeciesId::Dewspine, "Verdant Bulwark", 6,
     ElementId::Bloom, EffectSpec{.Heal = 5, .Shield = 5},
     "Restores 5 health and raises 5 shield. No damage."},
    {SpeciesId::Rimeclutch, SpeciesId::Voltmote, "Shatterbolt", 9,
     ElementId::Frost, EffectSpec{.StallTurns = 2, .Stall = StallScope::Longest},
     "Heavy hit that freezes the opponent's longest recovery even longer."},
    {SpeciesId::Glimmerquill, SpeciesId::Nullmoth, "Eclipse", 12,
     ElementId::Umbra, EffectSpec{.IgnoresShield = true},
     "Ignores shield entirely. The hardest shot in the game to set up."},
    {SpeciesId::Gritmaw, SpeciesId::Pyrelash, "Magmadrill", 10,
     ElementId::Ember, EffectSpec{.IgnoresShield = true},
     "Goes under the shield and detonates on the other side."},
    {SpeciesId::Skirlwing, SpeciesId::Thornwhistle, "Pollen Veil", 5,
     ElementId::Bloom, EffectSpec{.Heal = 4, .StallTurns = 1, .Stall = StallScope::All},
     "Heals you and stalls every slug the opponent is recovering. No damage."},
    {SpeciesId::Cinderling, SpeciesId::Mirebubble, "Fumewake", 8,
     ElementId::Venom, EffectSpec{.BurnTurns = 4},
     "Lights the fumes. Burns for four turns."},
    {SpeciesId::Pyrelash, SpeciesId::Voltmote, "Thunderhead", 14,
     ElementId::Spark, EffectSpec{},
     "The hardest raw hit in the game, and the slowest to reload."},
    {SpeciesId::Nullmoth, SpeciesId::Pyrelash, "Blacklight", 12,
     ElementId::Umbra, EffectSpec{.IgnoresShield = true},
     "Fire that casts no light. Goes straight through shield."},
    {SpeciesId::Dewspine, SpeciesId::Thornwhistle, "Springtide", 6,
     ElementId::Tide, EffectSpec{.Heal = 7},
     "Restores 7 health. No damage."},
    {SpeciesId::Bramblehusk, SpeciesId::Thornwhistle, "Thicketward", 7,
     ElementId::Bloom, EffectSpec{.Heal = 4, .Shield = 5},
     "Heals a little and shields a little. No damage."},
    {SpeciesId::Bramblehusk, SpeciesId::Gritmaw, "Rootsplit", 9,
     ElementId::Stone, EffectSpec{.IgnoresShield = true},
     "Roots crack the floor open under them. Ignores shield."},
    {SpeciesId::Bramblehusk, SpeciesId::Rimeclutch, "Frostbriar", 6,
     ElementId::Frost, EffectSpec{.Shield = 7, .StallTurns = 1, .Stall = StallScope::Longest},
     "Shields you and freezes their slowest slug a turn longer. No damage."},
    {SpeciesId::Dewspine, SpeciesId::Mirebubble, "Blackwater", 7,
     ElementId::Venom, EffectSpec{.BurnTurns = 3},
     "Poisons the pool. Burns for three turns."},
    {SpeciesId::Glimmerquill, SpeciesId::Mirebubble, "Blightlance", 10,
     ElementId::Lumen, EffectSpec{.IgnoresShield = true, .BurnTurns = 2},
     "Punches through shield and leaves it burning."},
    {SpeciesId::Gritmaw, SpeciesId::Rimeclutch, "Deepfreeze", 9,
     ElementId::Frost, EffectSpec{.IgnoresShield = true, .StallTurns = 2, .Stall = StallScope::Longest},
     "Goes under the shield and freezes their slowest slug solid."},
    {SpeciesId::Dewspine, SpeciesId::Rimeclutch, "Hailspring", 5,
     ElementId::Tide, EffectSpec{.Heal = 6, .StallTurns = 1, .Stall = StallScope::All},
     "Heals you and stalls every slug they are recovering. No damage."},
    {SpeciesId::Glimmerquill, SpeciesId::Voltmote, "Flashburn", 12,
     ElementId::Lumen, EffectSpec{.IgnoresShield = true},
     "Too fast to shield against. Ignores it entirely."},
    {SpeciesId::Nullmoth, SpeciesId::Skirlwing, "Duskgale", 6,
     ElementId::Umbra, EffectSpec{.Shield = 6, .StallTurns = 2, .Stall = StallScope::All},
     "Shields you and stalls everything they are recovering. No damage."},
    {SpeciesId::Cinderling, SpeciesId::Glimmerquill, "Sunflare", 9,
     ElementId::Lumen, EffectSpec{.BurnTurns = 3},
     "Ember carried on light. Burns for three turns."},
    {SpeciesId::Cinderling, SpeciesId::Slagmaw, "Magmavein", 11,
     ElementId::Ember, EffectSpec{.BurnTurns = 3},
     "Opens a vein of molten rock. Burns for three turns."},
    {SpeciesId::Gritmaw, SpeciesId::Slagmaw, "Bedrock Burst", 12,
     ElementId::Stone, EffectSpec{.IgnoresShield = true},
     "Detonates under the floor. Ignores shield."},
    {SpeciesId::Slagmaw, SpeciesId::Voltmote, "Fulgurite", 13,
     ElementId::Spark, EffectSpec{},
     "Glass-hard lightning. Nothing fancy, just enormous."},
    {SpeciesId::Chillblain, SpeciesId::Slagmaw, "Thermalshock", 12,
     ElementId::Frost, EffectSpec{.BurnTurns = 2},
     "Molten rock quenched at once. The cracking keeps going."},
    {SpeciesId::Thornwhistle, SpeciesId::Wispwick, "Lanternbloom", 7,
     ElementId::Bloom, EffectSpec{.Heal = 8},
     "Restores 8 health. No damage."},
    {SpeciesId::Glimmerquill, SpeciesId::Wispwick, "Dawnlance", 11,
     ElementId::Lumen, EffectSpec{.IgnoresShield = true},
     "Straight through the shield on a line of light."},
    {SpeciesId::Brinemoth, SpeciesId::Wispwick, "Tidelight", 6,
     ElementId::Tide, EffectSpec{.Heal = 5, .Shield = 6},
     "Heals a little and shields a little. No damage."},
    {SpeciesId::Nullmoth, SpeciesId::Wispwick, "Gloamward", 6,
     ElementId::Umbra, EffectSpec{.Shield = 8, .StallTurns = 1, .Stall = StallScope::Longest},
     "Shields you and holds their slowest slug back. No damage."},
    {SpeciesId::Hollowfang, SpeciesId::Nullmoth, "Voidbite", 13,
     ElementId::Umbra, EffectSpec{.IgnoresShield = true},
     "Takes a piece out of the dark, and out of them. Ignores shield."},
    {SpeciesId::Hollowfang, SpeciesId::Pyrelash, "Emberfang", 14,
     ElementId::Ember, EffectSpec{.IgnoresShield = true, .BurnTurns = 2},
     "Bites through the shield and leaves the wound burning."},
    {SpeciesId::Hollowfang, SpeciesId::Mirebubble, "Rotbite", 11,
     ElementId::Venom, EffectSpec{.IgnoresShield = true, .BurnTurns = 3},
     "Ignores shield and keeps eating for three turns."},
    {SpeciesId::Gallowspark, SpeciesId::Hollowfang, "Blackcurrent", 12,
     ElementId::Spark, EffectSpec{.IgnoresShield = true, .StallTurns = 2, .Stall = StallScope::Longest},
     "Through the shield, and their slowest slug stays down longer."},
    {SpeciesId::Brinemoth, SpeciesId::Dewspine, "Deepward", 8,
     ElementId::Tide, EffectSpec{.Heal = 6, .Shield = 7},
     "Restores 6 health and raises 7 shield. No damage."},
    {SpeciesId::Brinemoth, SpeciesId::Rimeclutch, "Floeguard", 7,
     ElementId::Frost, EffectSpec{.Shield = 9, .StallTurns = 1, .Stall = StallScope::All},
     "Shields you and stalls everything they are recovering. No damage."},
    {SpeciesId::Brinemoth, SpeciesId::Skirlwing, "Squallwall", 7,
     ElementId::Gale, EffectSpec{.Shield = 7},
     "Raises 7 shield out of moving water. No damage."},
    {SpeciesId::Gallowspark, SpeciesId::Voltmote, "Arcnet", 12,
     ElementId::Spark, EffectSpec{.StallTurns = 2, .Stall = StallScope::All},
     "Heavy hit that stalls every slug they are recovering."},
    {SpeciesId::Gallowspark, SpeciesId::Rimeclutch, "Deadfreeze", 10,
     ElementId::Frost, EffectSpec{.StallTurns = 3, .Stall = StallScope::Longest},
     "Their slowest slug is not coming back for a while."},
    {SpeciesId::Bramblehusk, SpeciesId::Gallowspark, "Livewire Thicket", 7,
     ElementId::Bloom, EffectSpec{.Shield = 8, .StallTurns = 1, .Stall = StallScope::Longest},
     "Shields you and earths their slowest slug. No damage."},
    {SpeciesId::Chillblain, SpeciesId::Mirebubble, "Frostrot", 10,
     ElementId::Venom, EffectSpec{.BurnTurns = 5},
     "Burns for five turns, the longest in the game."},
    {SpeciesId::Chillblain, SpeciesId::Dewspine, "Coldspring", 7,
     ElementId::Tide, EffectSpec{.Heal = 7},
     "Restores 7 health. No damage."},
    {SpeciesId::Chillblain, SpeciesId::Cinderling, "Steamburst", 10,
     ElementId::Ember, EffectSpec{.BurnTurns = 3},
     "Scalding steam. Burns for three turns."},
    {SpeciesId::Chillblain, SpeciesId::Glimmerquill, "Glacierlance", 12,
     ElementId::Lumen, EffectSpec{.IgnoresShield = true, .BurnTurns = 2},
     "Ignores shield and leaves frost burning in the wound."},
}};

/**
 * The fusion produced by firing Next immediately after Previous, if any.
 * Pair order never matters, so both arrangements are checked.
 */
inline constexpr const Fusion* FindFusion(SpeciesId Previous, SpeciesId Next) {
    if (Previous == SpeciesId::None) {
        return nullptr;
    }
    for (const Fusion& F : AllFusions) {
        if ((F.A == Previous && F.B == Next) || (F.B == Previous && F.A == Next)) {
            return &F;
        }
    }
    return nullptr;
}

}  // namespace Gargantua
