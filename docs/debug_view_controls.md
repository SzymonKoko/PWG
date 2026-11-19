# Debug View chunk and LOD controls (D-05)

Debug window → Render tab, section above the render counters. The controls exist in every build (also with
`PWG_DEBUG_VIEW_INSTRUMENTATION=OFF`) and do not need the Debug View switch. They are runtime state only: they are not saved
in a project and they go back to the defaults when PWG restarts. Design: thesis repository,
`docs/plan/debug_view_design.md` (Render tab, 8.3, 10, 11).

## Defaults

With every control at its default, chunk selection (LOD per chunk, culled chunks) and the rendered image are the same as before
D-05. Defaults: LOD enabled, scale 1, distances 220 / 450 / 900 / 1600, culling on, not frozen, tint and borders off.

## Controls

| Control | API (`ChunkManager`) | Runner parameter (D-11) | Effect |
|---|---|---|---|
| LOD enabled | `SetLodEnabled(bool)` | `lod.enabled` | `false` forces LOD0 on every chunk, in the main pass and in the three shadow cascades (the shadow pass draws every chunk at the LOD of the chunk). |
| Distance scale | `SetLodScale(float)` | `lod.scale` | Multiplies every distance. Accepted: finite, > 0, <= 100 (`LODManager::MaxScale`). Returns `false` and keeps the old value otherwise. |
| LOD distances | `SetLodDistances(std::array<float,4>)`, `ResetLodDistances()` | `lod.distances` (four numbers) | Chunks up to the first distance use LOD0, up to the second LOD1, up to the third LOD2, up to the fourth LOD3, the rest LOD4. Accepted: finite, >= 0, not decreasing. Returns `false` and keeps the old values otherwise. The fifth value of `LODDistances` (`lod4`) is not a switch distance and was never used by `SelectLOD`, so it is not settable. |
| Culling | `SetCullingEnabled(bool)` | `culling.enabled` | `false` draws every terrain chunk in the main pass (the shadow pass never culls). Vegetation has its own test and is not affected (design F10). |
| Culling freeze | `SetCullingFrozen(bool)` | (not a runner parameter) | Stops updating the frustum used by the chunk test. The visible set stays what the last frame before the freeze had, while the camera moves. LOD selection still follows the camera. |
| LOD tint | `SetLodTintEnabled(bool)` | – | Mixes a colour per LOD level (0 red, 1 orange, 2 yellow, 3 green, 4 blue) into the lit terrain colour. |
| Chunk borders | `SetChunkBordersEnabled(bool)` | – | Draws a white line, about 2 pixels wide, on the edges of every chunk. |

The distances are measured on the XZ plane from the camera position to the centre of the chunk (`ChunkManager::Update`).
The settings survive `ConfigureTerrain` (which rebuilds the LOD meshes).

## Cost of the overlays

One `vec3` uniform (`u_chunkDebug`) and one branch in `terrain.frag`, taken only when an overlay is on. The uniform is set once
per frame with the other shared terrain uniforms; while the tint is on it is set once more per drawn chunk (the LOD of the chunk).
The overlays are applied to the lit terrain only, not to the terrain debug modes (Viewport tab), and they appear in screenshots.

## Force LOD0

`LODDistances.h` warns that promoting many chunks to LOD0 at once stalled the GPU before the distances were reduced. With LOD
disabled the shadow pass submits every chunk of the grid at LOD0 in each cascade. Use a small grid (`terrain.chunk_count`) or a
low camera for E-10 (design 8.5).
