# Handoff

Everything a fresh session needs to continue Gargantua. Written at the point
where the rules were proven in Unreal and the first blockout existed but had
not been walked.

## What this is

An original 3D creature-collector. You explore caverns, find wild slugs, and
win them by duelling them. **The whole game is the order you fire in**: two
slugs fired back to back, if that pair has a fusion, resolve as the fusion
instead — far stronger than either alone. Cooldowns force rotation, rotation
decides which fusions you can still reach, and both players see everything.
No dice, no hidden information.

It was inspired by the tactics of creature-collecting games, and its combat
borrows one structural idea — that two creatures fired in sequence can combine.
Every species, fusion, character, place and line of text is original. Mechanics
are not copyrightable; names and characters are, which is why none are borrowed.

## Two repositories, deliberately linked

| Repo | Role |
|---|---|
| `altink14/gargantua` | The browser build. **Site retired**, code kept. Where the design was proven and where cavern layouts are authored. |
| `altink14/gargantua-3d` | The Unreal build. This one. |

`gargantua/tools/export-world-cpp.mjs` generates `gargantua-3d/Core/src/Caverns.h`.
Cavern layouts have **one** source. Change them in the browser repo, where
`npm run content` verifies reachability, then export.

## Current state

**Proven, by observation:**

- `Core/` — the rules in engine-free C++20. 44 checks pass. All four locked
  thresholds pass at 4000 matches per duel.
- The rules run inside Unreal. Editor startup logs a real duel between the two
  search policies.
- All three cavern layouts reproduce the browser build's walkable cell counts
  exactly: 20926, 23768, 25969.

**Built but never seen:**

- The cavern blockout and the player pawn. They compile and the editor loads
  them, but **nobody has put a `GargCavern` in a level and walked it.** This is
  the immediate next thing, and it needs the editor GUI.

**Not started:** duels in 3D, slug models, any art.

## The four locked thresholds

Fixed before the combat system was written. `Core/build/depth.exe` measures
them. **They are not to be lowered to make a run pass.** A failure is a signal
to redesign the combat.

```
tactical beats greedy   >= 70%      planning beats grabbing the biggest number
greedy beats random     >= 70%      playing well beats playing badly
most-used species       <= 55%      no single slug dominates
depth-6 beats depth-2   >= 60%      looking further ahead keeps paying
```

Threshold 4 was changed **once**, with the owner's sign-off, from "median
viable moves >= 3" — which measured whether a depth-4 search finds one clearly
best move, true of nearly any deterministic game including chess. The reasoning
is recorded above `Thresholds` in `Core/tools/DepthHarness.cpp`, and the
retired metric is still printed in the browser build so the change is auditable.

## How this project is worked on

The owner set these and they have caught real bugs. Keep them.

- **Verify by observation, not by reasoning.** "It compiles" is not "it works".
  Say plainly what was seen and what was not.
- **Different evidence than the thing being checked.** If a subagent ran a unit
  test, run the actual flow. Re-running their command is transcription.
- **Never weaken a test to make it pass.** If a test is wrong, say why and let
  the owner decide.
- **Lead with the bad news.** Broken or uncertain things go in the first
  sentence.
- **One commit per verified work item.** Never commit unverified work without
  saying so in the message.

## Toolchain

- Unreal Engine 5.8
- Visual Studio Build Tools, "Desktop development with C++"
- **MSVC 14.44 or newer.** UnrealBuildTool bans 14.40 through 14.43.
- **.NET Framework 4.8 SDK**, or the editor target will not configure.

```
Core\build.bat                       tests.exe, depth.exe, content.exe
"...\UE_5.8\Engine\Build\BatchFiles\Build.bat" Gargantua3DEditor Win64 Development -Project="...\Gargantua3D.uproject"
```

## Gotchas already paid for

- **No `using namespace` in module .cpp files.** Unreal compiles them as one
  merged translation unit, so it leaks into everything after it. This produced
  a build error inside Epic's own headers. Use a namespace alias.
- **`BuildSettingsVersion` must match the installed engine** (V7), or targets
  sharing build products with UnrealEditor are refused.
- **Do not enable exceptions.** Core never throws by design, and enabling them
  forces a PCH variant engine headers are not warning-clean in.
- **The editor re-adds an AndroidFileServer `SecurityToken`** to
  `Config/DefaultEngine.ini`. Strip it before committing.
- **Scale is 5cm per layout unit**, declared once in `Core/src/World.h`. Picked
  by arithmetic, never walked. Expect to change it.

## Next steps, in order

1. **Walk the blockout.** New Level → Empty, drag in a `GargCavern`, press Play.
   Judge the scale. This is the one open question nothing else can answer.
2. Slug and trainer actors that trigger a duel on proximity, handing control to
   `Core` and reporting the result back.
3. A duel interface in 3D.
4. Asset packs from Fab for cavern kit and creatures. Budget is a few hundred.

## Open questions for the owner

- Does the scale feel right underfoot?
- Losing a wild duel deletes that slug permanently. Their approved design, but
  the game has never warned the player. Still unresolved.
- Console distribution needs an approved developer account and is realistically
  a destination, not a year-one step. Steam first.
