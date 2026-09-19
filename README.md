# Gargantua 3D

The Unreal build of Gargantua. Original setting; see `Core/` for the rules.

Picking this up fresh? Read [HANDOFF.md](HANDOFF.md) first — current state,
what is proven versus merely built, the locked balance thresholds, and the
toolchain gotchas already paid for.

## Core/ — the rules, with no engine in them

`Core/` is plain C++17 with zero Unreal dependencies: the combat rules, the
element wheel, every species and fusion, and the search AI. It compiles and
runs on its own, and its tests and balance harness run without opening an
editor.

That separation is deliberate and worth keeping. It is what let this design be
proven before any of it was drawn, and it is the reason the port from the
original TypeScript was a translation rather than a rewrite.

The design it implements — and the numbers it has to match — come from
`../gargantua`, the original browser build.

## Building the game

Needs Unreal Engine 5.8, Visual Studio Build Tools with the "Desktop
development with C++" workload, **MSVC 14.44 or newer** (UnrealBuildTool bans
14.40 through 14.43 over known compiler bugs), and the **.NET Framework 4.8
SDK**, without which the editor target will not configure.

From the project root:

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" Gargantua3DEditor Win64 Development -Project="%CD%\Gargantua3D.uproject"
```

On startup the module plays a real duel between the two search policies and
logs the result, so that "the rules work inside Unreal" is something you read
rather than something assumed because the module linked:

```
LogGargantua: Gargantua rules: 18 species, 44 fusions. Wheel problems: 0.
Duel finished on turn 7 (side 0 won), 4 fusions fired.
```

## Building the core

Needs MSVC (Visual Studio Build Tools with the "Desktop development with C++"
workload). From a shell with the compiler on PATH:

```
Core\build.bat
```

That produces `Core/build/tests.exe` and `Core/build/depth.exe`.
