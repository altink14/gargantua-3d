/**
 * Checks the caverns are actually walkable.
 *
 * Eyeballing coordinates is how a slug ends up sealed inside a pillar nobody
 * can reach, or a door leads somewhere with no door back. Every marker, lamp
 * and doorway is checked against the real collision code and a flood fill from
 * the point a player actually arrives at.
 *
 * Every check here caught something real in the browser build: a slug behind a
 * wall, a lamp buried in rock advertising a passage that was not there, and an
 * arrival point sitting inside a doorway, which bounced the player back and
 * forth between two caverns forever.
 *
 * Build with Core\build.bat, then run build\content.exe.
 */

#include <cmath>
#include <cstdio>
#include <string>
#include <unordered_set>
#include <vector>

#include "Caverns.h"
#include "Fusions.h"
#include "Species.h"
#include "World.h"

namespace G = Gargantua;

namespace {

constexpr float Step = 10.0f;
int Problems = 0;

void Check(bool Ok, const std::string& Label, const std::string& Detail) {
    if (!Ok) {
        ++Problems;
    }
    std::printf("   %s %-26s %s\n", Ok ? "ok  " : "FAIL", Label.c_str(), Detail.c_str());
}

/** Where the player first stands in a cavern: its start, or where a door lands. */
std::vector<G::Point> ArrivalPoints(const G::Biome& B) {
    std::vector<G::Point> Points;
    if (B.Id == G::FirstBiome.Id) {
        Points.push_back({400.0f, 300.0f});
    }
    for (const G::Biome* Other : G::AllBiomes) {
        for (int I = 0; I < Other->ExitCount; ++I) {
            if (Other->Exits[I].To == B.Id) {
                Points.push_back(Other->Exits[I].Entry);
            }
        }
    }
    return Points;
}

struct Reach {
    std::unordered_set<long long> Cells;

    static long long Key(int X, int Y) { return static_cast<long long>(X) * 100000 + Y; }

    bool Reachable(float X, float Y) const {
        for (int Ox = -1; Ox <= 1; ++Ox) {
            for (int Oy = -1; Oy <= 1; ++Oy) {
                const int Gx = static_cast<int>(std::lround((X + Ox * Step) / Step));
                const int Gy = static_cast<int>(std::lround((Y + Oy * Step) / Step));
                if (Cells.count(Key(Gx, Gy)) > 0) {
                    return true;
                }
            }
        }
        return false;
    }
};

Reach FloodFill(const G::Biome& B) {
    Reach R;
    std::vector<std::pair<int, int>> Queue;

    for (const G::Point& P : ArrivalPoints(B)) {
        const int Sx = static_cast<int>(std::lround(P.X / Step));
        const int Sy = static_cast<int>(std::lround(P.Y / Step));
        if (!G::Blocked(B, Sx * Step, Sy * Step) && R.Cells.insert(Reach::Key(Sx, Sy)).second) {
            Queue.emplace_back(Sx, Sy);
        }
    }

    while (!Queue.empty()) {
        const auto [X, Y] = Queue.back();
        Queue.pop_back();
        const int Dx[] = {1, -1, 0, 0};
        const int Dy[] = {0, 0, 1, -1};
        for (int I = 0; I < 4; ++I) {
            const int Nx = X + Dx[I];
            const int Ny = Y + Dy[I];
            if (R.Cells.count(Reach::Key(Nx, Ny)) > 0) {
                continue;
            }
            if (G::Blocked(B, Nx * Step, Ny * Step)) {
                continue;
            }
            R.Cells.insert(Reach::Key(Nx, Ny));
            Queue.emplace_back(Nx, Ny);
        }
    }
    return R;
}

std::string Coords(float X, float Y) {
    char Buf[64];
    std::snprintf(Buf, sizeof(Buf), "(%g, %g)", X, Y);
    return Buf;
}

}  // namespace

int main() {
    std::printf("Gargantua content report — %d species, %d fusions, %d caverns\n",
                G::SpeciesCount, static_cast<int>(G::AllFusions.size()),
                static_cast<int>(G::AllBiomes.size()));

    for (const G::Biome* Bp : G::AllBiomes) {
        const G::Biome& B = *Bp;
        const Reach R = FloodFill(B);

        std::printf("\n%s — %gx%g, %d walkable cells reachable\n", std::string(B.Name).c_str(),
                    B.Bounds.W, B.Bounds.H, static_cast<int>(R.Cells.size()));

        for (const G::Point& P : ArrivalPoints(B)) {
            // Landing inside a doorway sends the player straight back where they
            // came from, forever.
            const G::Exit* Inside = nullptr;
            for (int I = 0; I < B.ExitCount; ++I) {
                if (G::CircleHitsRect(P.X, P.Y, G::PlayerRadius, B.Exits[I].Bounds)) {
                    Inside = &B.Exits[I];
                }
            }
            Check(!G::Blocked(B, P.X, P.Y) && Inside == nullptr, "arrival point",
                  Inside ? Coords(P.X, P.Y) + " LANDS INSIDE " + std::string(Inside->Id)
                         : Coords(P.X, P.Y));
        }

        for (int I = 0; I < B.EncounterCount; ++I) {
            const G::Encounter& E = B.Encounters[I];
            Check(!G::Blocked(B, E.X, E.Y) && R.Reachable(E.X, E.Y), std::string(E.Id),
                  std::string(G::SpeciesOf(E.Species).Name));
        }

        for (int I = 0; I < B.TrainerCount; ++I) {
            const G::Trainer& T = B.Trainers[I];
            char Detail[128];
            std::snprintf(Detail, sizeof(Detail), "%s (%d slugs, depth %d)",
                          std::string(T.Name).c_str(), T.LoadoutCount, T.Depth);
            Check(!G::Blocked(B, T.X, T.Y) && R.Reachable(T.X, T.Y), std::string(T.Id), Detail);
        }

        for (int I = 0; I < B.PassageCount; ++I) {
            const G::Point& P = B.Passages[I];
            Check(!G::Blocked(B, P.X, P.Y) && R.Reachable(P.X, P.Y),
                  "passage-lamp-" + std::to_string(I), Coords(P.X, P.Y));
        }

        for (int I = 0; I < B.ExitCount; ++I) {
            const G::Exit& E = B.Exits[I];
            const float Cx = E.Bounds.X + E.Bounds.W * 0.5f;
            const float Cy = E.Bounds.Y + E.Bounds.H * 0.5f;
            const G::Biome* Target = G::FindBiome(E.To);
            Check(Target != nullptr && !G::Blocked(B, Cx, Cy) && R.Reachable(Cx, Cy),
                  std::string(E.Id), "-> " + std::string(E.To));

            if (Target != nullptr) {
                bool Back = false;
                for (int J = 0; J < Target->ExitCount; ++J) {
                    if (Target->Exits[J].To == B.Id) {
                        Back = true;
                    }
                }
                Check(Back, std::string(E.Id) + " return",
                      Back ? "a way back exists" : "NO WAY BACK from " + std::string(E.To));
            }
        }

        // Two markers close enough to overlap would fight over the same footstep.
        std::vector<std::pair<std::string, G::Point>> Marks;
        for (int I = 0; I < B.EncounterCount; ++I) {
            Marks.push_back({std::string(B.Encounters[I].Id), {B.Encounters[I].X, B.Encounters[I].Y}});
        }
        for (int I = 0; I < B.TrainerCount; ++I) {
            Marks.push_back({std::string(B.Trainers[I].Id), {B.Trainers[I].X, B.Trainers[I].Y}});
        }
        for (std::size_t I = 0; I < Marks.size(); ++I) {
            for (std::size_t J = I + 1; J < Marks.size(); ++J) {
                const float D = std::hypot(Marks[I].second.X - Marks[J].second.X,
                                           Marks[I].second.Y - Marks[J].second.Y);
                if (D < 200.0f) {
                    Check(false, Marks[I].first + " vs " + Marks[J].first,
                          "only " + std::to_string(static_cast<int>(D)) + " units apart");
                }
            }
        }
    }

    // Every slug must be gettable somewhere, or the codex can never be filled.
    std::unordered_set<int> Obtainable;
    for (const G::SpeciesId Id : G::StarterSpecies) {
        Obtainable.insert(static_cast<int>(Id));
    }
    for (const G::Biome* B : G::AllBiomes) {
        for (int I = 0; I < B->EncounterCount; ++I) {
            Obtainable.insert(static_cast<int>(B->Encounters[I].Species));
        }
        for (int I = 0; I < B->TrainerCount; ++I) {
            Obtainable.insert(static_cast<int>(B->Trainers[I].Reward));
        }
    }
    std::printf("\n%d of %d species obtainable across %d caverns.\n",
                static_cast<int>(Obtainable.size()), G::SpeciesCount,
                static_cast<int>(G::AllBiomes.size()));
    for (const G::Species& S : G::AllSpecies) {
        if (Obtainable.count(static_cast<int>(S.Id)) == 0) {
            ++Problems;
            std::printf("   FAIL not obtainable anywhere: %s\n", std::string(S.Key).c_str());
        }
    }

    int Dead = 0;
    for (const G::Fusion& F : G::AllFusions) {
        if (Obtainable.count(static_cast<int>(F.A)) == 0 || Obtainable.count(static_cast<int>(F.B)) == 0) {
            ++Dead;
            ++Problems;
            std::printf("   FAIL never fireable: %s\n", std::string(F.Name).c_str());
        }
    }
    std::printf("%d of %d fusions can actually be fired.\n",
                static_cast<int>(G::AllFusions.size()) - Dead,
                static_cast<int>(G::AllFusions.size()));

    if (Problems > 0) {
        std::printf("\n%d PROBLEM(S) FOUND\n", Problems);
        return 1;
    }
    std::printf("\nAll caverns check out.\n");
    return 0;
}
