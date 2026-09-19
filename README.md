# Gargantua 3D

The Unreal build of Gargantua. Original setting; see `Core/` for the rules.

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

## Building the core

Needs MSVC (Visual Studio Build Tools with the "Desktop development with C++"
workload). From a shell with the compiler on PATH:

```
Core\build.bat
```

That produces `Core/build/tests.exe` and `Core/build/depth.exe`.
