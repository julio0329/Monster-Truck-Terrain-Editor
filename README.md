# Monster Truck Terrain Editor

An in-game terrain editor for building monster truck arenas in Unreal Engine 5.8.
You sculpt a heightfield with brushes and drop in parametric jump ramps while the game is running.

> **Status: untested scaffold.** This code was written without access to an Unreal build
> environment, so it has not been compiled or run. Expect to fix a few compile errors on the
> first build, especially around Unreal API signatures that shift between engine versions.

## Setup

1. Install Unreal Engine 5.8 and a C++ toolchain (Visual Studio with the "Game development with C++" workload on Windows).
2. Install [Git LFS](https://git-lfs.com) before adding any `.uasset` or `.umap` files. `.gitattributes` already tracks them.
3. Right-click `MonsterTruckTerrain.uproject` and choose **Generate Visual Studio project files**.
4. Open the generated `.sln`, build the `MonsterTruckTerrainEditor` target (Development Editor), and launch.
5. In the editor, create or open any level (File > New Level > Basic works) and press **Play**.
   `Config/DefaultEngine.ini` sets the project's default game mode, so no level setup is needed.
   The game mode spawns the terrain and a fly camera for you.

## Controls

| Input | Action |
| --- | --- |
| Left mouse | Apply the brush (hold Alt to lower while the Raise brush is active) |
| `1` `2` `3` `4` | Brush: Raise, Lower, Smooth, Flatten |
| `[` `]` | Brush radius down / up |
| `-` `=` | Brush strength down / up |
| `R` | Place a ramp at the cursor, facing the camera heading |
| `Z` `X` | Ramp height down / up |
| `C` `V` | Ramp length down / up |
| `B` `N` | Ramp width down / up |
| `K` `L` | Ramp curve down / up (1 = straight wedge, higher = steeper toward the lip) |
| `U` | Undo (last 30 strokes or placements) |
| `F5` / `F9` | Save / load the arena (slot `Arena1`) |
| Hold right mouse + `W` `A` `S` `D` `Space` | Fly the camera |

## How it works

- **`ATerrainActor`** stores the terrain as a plain height array and splits it into chunks (default 4 x 4 chunks of 32 x 32 cells, 100 uu per cell, so about 128 m across). A brush edit changes the array and rebuilds only the chunks it touched. Normals are computed from the global array, so chunk seams stay smooth.
- **`ARampActor`** builds a closed solid from `FRampParams`. Drive direction is local +X, with the launch lip at the far end.
- **`UTerrainSaveGame`** stores the height array and every ramp's transform and parameters. It includes a version number for future migrations.
- **`AMonsterTruckController`** handles input by polling keys directly, so the project needs no input assets.
- Terrain and ramps use `UProceduralMeshComponent` with async collision cooking.

## Known limitations and next steps

- No truck yet. Add a Chaos Vehicles pawn and a play-mode toggle that spawns it on the arena you built.
- Terrain is a heightfield, so no caves or overhangs. Ramps cover jumps.
- Collision is rebuilt for each edited chunk while you paint. If sculpting hitches, lower `ChunkCells` or move chunk rebuilds off the game thread.
- `UProceduralMeshComponent` is the simplest option; `UDynamicMeshComponent` with Geometry Script is the more capable alternative if you outgrow it.
- Ramp placement is not snapped to a grid or to other ramps.
- A save made with a different terrain grid size is rejected on load.
