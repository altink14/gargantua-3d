#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

#include "Species.h"

namespace Gargantua {

/**
 * The world, as geometry.
 *
 * These layouts were authored and balanced for the browser build, where every
 * marker and doorway was checked by a flood fill from the point a player
 * actually arrives at. Keeping them here — in the engine-free core rather than
 * in Unreal — means that checking still runs without an editor, which is how
 * a slug sealed inside a pillar got caught the first time.
 *
 * Coordinates are layout units, the same numbers the browser build used. The
 * renderer scales them; the rules and the checks never do.
 */

/**
 * Layout units to Unreal centimetres.
 *
 * At 5cm a cavern is about 100 by 75 metres, its divider walls are two metres
 * thick, and the mouths through them are ten metres wide — a space you walk
 * through rather than squeeze along. The original numbers were tuned for a
 * top-down view; this is the one place that opinion about scale lives, so it
 * can be changed in one edit if the space feels wrong underfoot.
 */
inline constexpr float UnitsToCentimetres = 5.0f;

struct Rect {
    float X = 0.0f;
    float Y = 0.0f;
    float W = 0.0f;
    float H = 0.0f;
};

struct Point {
    float X = 0.0f;
    float Y = 0.0f;
};

/** A wild slug standing somewhere in the caverns, waiting to be duelled. */
struct Encounter {
    /** Stable id. Recorded in the save once resolved so it does not respawn. */
    std::string_view Id;
    SpeciesId Species = SpeciesId::None;
    float X = 0.0f;
    float Y = 0.0f;
};

/**
 * A rival slinger.
 *
 * Trainers are where this game's design actually lives. A wild slug holds one
 * slug and can only fire it or wait; a trainer rotates a full team, which is
 * the only situation where sequencing, fusion choice and elemental exposure
 * all matter at once. Wild duels fill your blaster. Trainers test it.
 */
struct Trainer {
    std::string_view Id;
    std::string_view Name;
    /** Shown under the name. Sets expectations before a fight. */
    std::string_view Title;
    std::array<SpeciesId, 5> Loadout{};
    int LoadoutCount = 0;
    /** Plies the opponent searches. This is the difficulty dial. */
    int Depth = 4;
    std::string_view Greeting;
    std::string_view DefeatLine;
    /** Said when the player loses. Losing is rematchable, so never discouraging. */
    std::string_view VictoryLine;
    SpeciesId Reward = SpeciesId::None;
    float X = 0.0f;
    float Y = 0.0f;
};

/**
 * A way out of one cavern and into the next.
 *
 * Travel is a place you walk into rather than a menu, so the caverns read as
 * one connected world. Every exit has a matching one pointing back; nothing
 * here is one-way, and the content report fails if that stops being true.
 */
struct Exit {
    std::string_view Id;
    Rect Bounds;
    std::string_view To;
    /** Where you stand when you arrive. Must be clear of that cavern's walls. */
    Point Entry;
    std::string_view Label;
};

/** What a cavern is made of, for the renderer. */
enum class Mood : std::uint8_t { Stone, Molten, Drowned };

struct Biome {
    std::string_view Id;
    std::string_view Name;
    std::string_view Subtitle;
    Rect Bounds;
    const Rect* Walls = nullptr;
    int WallCount = 0;
    const Encounter* Encounters = nullptr;
    int EncounterCount = 0;
    const Trainer* Trainers = nullptr;
    int TrainerCount = 0;
    const Exit* Exits = nullptr;
    int ExitCount = 0;
    /** One ore lamp in the mouth of every gap. A passage nobody can see is a wall. */
    const Point* Passages = nullptr;
    int PassageCount = 0;
    Mood Character = Mood::Stone;
};

inline constexpr float PlayerRadius = 14.0f;
/** How close the player must be for an encounter to trigger. */
inline constexpr float EncounterRadius = 34.0f;

/** Axis-aligned circle/rect overlap, used for walls, markers and doorways. */
inline constexpr bool CircleHitsRect(float Cx, float Cy, float R, const Rect& Box) {
    const float NearestX = Cx < Box.X ? Box.X : (Cx > Box.X + Box.W ? Box.X + Box.W : Cx);
    const float NearestY = Cy < Box.Y ? Box.Y : (Cy > Box.Y + Box.H ? Box.Y + Box.H : Cy);
    const float Dx = Cx - NearestX;
    const float Dy = Cy - NearestY;
    return Dx * Dx + Dy * Dy < R * R;
}

/** Whether a point is solid: outside the bounds, or inside any wall. */
inline bool Blocked(const Biome& B, float X, float Y, float Radius = PlayerRadius) {
    if (X < Radius || Y < Radius || X > B.Bounds.W - Radius || Y > B.Bounds.H - Radius) {
        return true;
    }
    for (int I = 0; I < B.WallCount; ++I) {
        if (CircleHitsRect(X, Y, Radius, B.Walls[I])) {
            return true;
        }
    }
    return false;
}

}  // namespace Gargantua
