# Debug View memory statistics (D-04)

Code: `src/debug/memoryStats.{h,cpp}`, Memory tab (`DebugView::DrawMemoryTab` in `src/debug/debugViewPanel.cpp`), `DebugView::RecordMemorySnapshot`, runner key `memory_snapshots` (`debug_view_runner.md`). Rows in events.csv: `debug_view_export.md`, section "Memory snapshots (D-04)".
Design: thesis repository, `docs/plan/debug_view_design.md` section 5.7 (the three methods), 7.3 (`memory_snapshot`), 9 (E-05).

Everything here is compiled only with `PWG_DEBUG_VIEW_INSTRUMENTATION=ON` (the default). With `OFF` the allocation hooks are empty macros (no code, and `pwg::Texture` loses its two bookkeeping members), the Memory tab says so, and no `memory_snapshot` row is written.

## Three numbers, three methods

The Memory tab (Debug window) shows each value with the method that produced it. They are never added to or subtracted from each other.

| Label | Method | What it is | What it is not |
|---|---|---|---|
| `own_accounting` | counters updated by the engine's allocation sites (below) | nominal bytes of the objects the engine counts | not all GPU memory; not what the driver really reserved (no padding, alignment or driver copies) |
| `os_process` | `GetProcessMemoryInfo`: `PrivateUsage`, `WorkingSetSize` (and `PeakWorkingSetSize` in the tab) | CPU-side memory of this process (heap, mapped files, driver user-mode allocations) | not GPU memory |
| `driver_nvx` | `GL_NVX_gpu_memory_info`: `DEDICATED_VIDMEM_NVX` and `CURRENT_AVAILABLE_VIDMEM_NVX`, used = dedicated - available | VRAM in use on the whole GPU: this process, other processes (desktop, browser) and the driver's own use | not this process only; not comparable with `own_accounting` by subtraction |
| `unavailable` | extension not in the extension list | the VRAM rows have an empty value | |

`GL_NVX_gpu_memory_info` is a vendor extension (NVIDIA) and not part of the OpenGL specification; the values are in KiB. The extension is checked once, from the extension list.

## What own accounting covers

Exactly these allocations, and only these:

| Class | Hook | Bytes |
|---|---|---|
| texture | `pwg::Texture`: the three loading/creating constructors, `~Texture`, `UpdateData`, `UpdateFloatData` when the size changes | width x height x bytes per texel of the internal format (`R8` 1, `R32F` 4, `RG32F` 8, `RGBA8` and `SRGB8_ALPHA8` 4, `RGBA16F` 8, `RGBA32F` 16); for a texture with generated mipmaps the sum of the whole mip chain (each level halved, rounded down, to 1 x 1) |
| particle SSBO | `GpuParticleErosion` constructor and destructor | particle capacity x 32 B |
| height PBOs | `ChunkManager::QueueHeightCacheReadback`, destructor | world size squared x 4 B per readback slot, when the slot's buffer is (re)specified |
| instance VBOs | `Mesh::DrawInstanced` (growth), `~Mesh` | the capacity of the instance buffer |

**Not counted:** mesh vertex and index buffers (terrain chunks, vegetation and every other `Mesh`), framebuffers and their depth textures, shadow maps, cube maps, texture arrays, the erosion, thermal and water textures that are created with `glTexStorage2D` outside `pwg::Texture`, the ImGui font and backend textures, the scene-depth copy, and everything the driver allocates for itself. The UI and this document say so; the design (5.7) lists the same four classes.

Notes on the numbers:

- The counters are relaxed atomics and count whether the Debug View is enabled at run time or not (most allocations happen before it is switched on). Cost: one or two atomic operations per allocation event, nothing per frame (a same-size `UpdateFloatData` only compares two integers).
- **Destroyed textures are not freed by the engine.** `pwg::Texture::~Texture` has `glDeleteTextures` commented out, so the GL storage of a destroyed `pwg::Texture` stays in the driver. The accounting reports this instead of pretending the memory came back: the bytes leave `texture_live` when the object is destroyed and are added to `texture_orphaned` (cumulative, shown in the tab and as the `texture_orphaned` row). Engine behaviour is unchanged.
- Mip chains: the design says "x 4/3 when mipmapped". The exact chain sum is used instead (for 2048 x 2048 it differs from 4/3 by under 0.01 %).
- `os_process` and `driver_nvx` are read on request (the tab refreshes them four times per second while it is drawn; the runner reads them at its snapshot points). Own accounting is read from the atomics.

## Where snapshots are taken

- Memory tab: the numbers are live. The button "Add snapshot to the CSV window" adds a `memory_snapshot` set (`point=manual`) while a CSV window records.
- Runner: `memory_snapshots: true` in the config, see `debug_view_runner.md` (`prepare_start`, `after:<entry>`, `measure_start`, `measure_end`). Example: `examples/experiments/memory_example.yaml`.
- A snapshot does not wait for the GPU. After work that is still queued (fixed erosion steps, an upload) the driver's VRAM figure may lag; graph runs and hydrology end with a synchronous readback, so they are complete by the next snapshot.

## Checks (2026-11-20, RTX 3060 Laptop, driver 610.91; functional checks, not thesis data)

A temporary harness (not committed) created objects of known size and compared the counters with an independent calculation and with the `driver_nvx` used-VRAM delta (`glFinish` before each reading; five idle readings in a row were identical, so the reading itself is stable).

| Check | Result |
|---|---|
| Textures of known size (`R32F` 1024^2, 4096^2; `RGBA32F` 2048^2; `RGBA16F` 4096^2; `RGBA8` 2048^2, storage forced with `glClearTexImage`) | own delta equals width x height x bytes per texel exactly (4 MiB, 64 MiB, 64 MiB, 128 MiB, 16 MiB). VRAM delta equal to the own delta in all five cases with a small world loaded; in a first run with the default 3999^2 world loaded (VRAM at 95 to 96 % of 6 GiB) two of the five VRAM deltas were smaller (1.5 MiB for the 64 MiB `R32F`, 64 MiB for the 128 MiB `RGBA16F`), so under VRAM pressure the driver value does not follow an allocation one to one |
| Mipmapped image (`welcome1.png`, 1672 x 941, `SRGB8_ALPHA8`) | own delta 8 387 888 B = exact mip chain; level 0 alone 6 293 408 B; the design's 4/3 estimate 8 391 210 B. VRAM delta 9 240 576 B (driver padding and alignment: 852 688 B more than counted) |
| Instance VBO (`Mesh::DrawInstanced`, 262 144 then 524 288 `mat4`) | own 16 777 216 B, grown to 33 554 432 B (the growth rule `max(required, capacity x 1.5)`); VRAM delta equal; after destroying the mesh own and VRAM back to the start |
| Particle SSBO | counted capacity x 32 B equals the formula (world 3999: 7 996 000 B; 325: 1 048 576 B), one object; after `ConfigureTerrain` to another size the old buffer is released and the new one counted (world 3999 to 3483: 7 996 000 to 6 065 632 B, still one object) |
| Height readback PBOs | 2 slots of world^2 x 4 B (195: 2 x 152 100 B); after `ConfigureTerrain` to 325 the slots were re-specified: 2 x 422 500 B. The destructor path (release) runs at shutdown only and was not observed |
| Freeing | after `reset()` of a texture, live bytes return to the start value exactly, `texture_orphaned` grows by the same bytes, and VRAM does **not** return (0 delta), because the engine does not delete the GL texture. With a manual `glDeleteTextures` the VRAM delta returned by the full size in all five cases. Instance VBOs and the SSBO (real `glDeleteBuffers`) return in both counters |

What this shows: the counters follow the objects they cover, byte for byte, and the freed side works. What it does not show: how much of the VRAM in use these objects are. In the runs above own accounting was 0.9 to 3.1 GB (texture live) while the driver reported 5.6 to 6.2 GB of 6 GiB used (this machine also has a display and other programs on the GPU); the difference is mesh buffers, framebuffers, shadow maps, erosion and water textures, the leaked textures below, other processes and the driver, and cannot be split from these numbers.

## Findings for the analysis (E-05)

1. **`~Texture` leaks GPU memory** (`glDeleteTextures` is commented out since the first version of `texture.cpp`). Every destroyed `pwg::Texture` stays in the driver. `ChunkManager::ConfigureTerrain`, which a project load or `terrain.chunk_*` triggers, destroys the whole pipeline's textures: in one harness run `texture_orphaned` grew from 0.38 GB to 2.04 GB in one `ConfigureTerrain` call (world 3999 to 3483). Not changed here (engine behaviour and results must not change); the accounting makes it visible.
2. **`driver_nvx` VRAM is cumulative over the process life.** The application builds the default 31 x 129 world at start-up (about 3 GB of `pwg::Texture`), and the leak above means a run that reloads the project for every unit (as the runner does) carries the memory of earlier units. Snapshots of the second unit of `memory_example.yaml` showed a larger `texture_orphaned` (3.40 GB) than the first (2.80 GB). For E-05 use `texture_live` (own accounting) as the per-world figure, and a fresh process per variant if a clean `driver_nvx` figure is wanted; the card decides.
3. Process `private_usage` includes the same history (heap that was freed but not returned to the OS, driver user-mode memory), so it is not a per-world figure either.

## Overhead (measured, same machine)

- Hooks: 2 000 000 `OnAlloc` + `OnRelease` pairs in a loop: 30 to 33 ns per pair, one `OnResize` about 20 ns (includes the loop). They run at allocation events only (texture and buffer creation, instance buffer growth); nothing is added per frame. `UpdateFloatData` (called from `chunkManager.cpp` and `terrainComputePipeline.cpp`) reaches the accounting only when the size changes, so the same-size path is unchanged.
- `TakeSnapshot` (7 atomic loads, `GetProcessMemoryInfo`, two `glGetIntegerv`): median 3.8 to 4.0 us over 1000 calls, p99 13 to 27 us, largest 68 to 254 us; `GetProcessMemory` alone 0.2 us. The Memory tab does this four times per second while it is drawn; the runner does it at its snapshot points only.
- `frame_ms`, `frames_example.yaml` (1280 x 720 benchmark, 4 units of 120 path frames), 4 rounds alternating instrumentation OFF and ON (Debug View enabled): median of the 16 window medians 12.06 ms (OFF, range 10.89 to 13.18) and 11.91 ms (ON, range 10.83 to 27.09; three windows of the first two ON rounds were at 20 to 27 ms while the other 13 were 10.8 to 12.7). The machine was busy with other work in those rounds, so the spread is larger than any difference between the configurations; no difference is measurable here. Not a statement of zero cost.
- The `frame_ms` level here (about 11 to 12 ms) is higher than the 7.9 ms of the D-11 hand-over for the same config; the GPU was at 95 % VRAM use during these runs.
