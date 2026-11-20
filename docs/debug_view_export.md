# Debug View export formats (D-09 performance, D-07 screenshot, D-08 masks, D-10 camera path)

Format `pwg-debugview-csv`, `format_version` 1. Code: `src/debug/csvWriter.*`, `src/debug/runMetadata.*`.
Design: thesis repository, `docs/plan/debug_view_design.md` sections 7.1 to 7.3.

## Recording a window

Debug window → Performance tab → CSV export.

1. Set the output folder (default `perf_exports`, relative to the working directory, which is `build/` when PWG is started as documented) and the number of frames (0 = until Stop).
2. Start recording. The window begins with the next frame. Debug View switch, benchmark mode, render size, ring frames K, cascade timing and the Simulation tab's Clear button are locked until the export is written, because the metadata header describes the whole window.
3. Stop and export (or wait for the frame count). The recorder keeps rendering until every frame of the window has its GPU results (K frames) and no GPU event that started inside the window is pending (at most 600 frames). Then it writes the files, at the start of a frame outside the window. Nothing is written to disk inside the window.

Abort drops the window. Closing the application during a recording drops it too.

The experiment runner (D-11, `debug_view_runner.md`) uses the same code through `DebugView::StartRecording(RunInfo, root, frames)` and `StopRecording()`.

The export runs in every build. With `PWG_DEBUG_VIEW_INSTRUMENTATION=OFF`, or with the Debug View switched off, frames.csv holds the frame timing (`frame_ms`, `cpu_*_ms`, `t_ms`) and the GPU and counter columns are empty; events.csv has no rows.

## Files

`<output folder>/<run id>/`, run id = local start time and a counter, e.g. `2026-11-17T14-05-11_m01` (`m` = manual export; the runner passes its own run id).

| File | Content |
|---|---|
| `frames.csv` | one row per frame of the window |
| `events.csv` | one row per measured value of a one-shot event (long format) |
| `summary.csv` | one row per metric |
| `run.yaml` | `metadata` (the header below), `result` (row counts, warnings), `files` |

All CSV files: UTF-8, comma separator, `.` decimal point, `\n` line ends. They start with the metadata as comment lines `# key: value`, then one header row. Read them with `pandas.read_csv(path, comment="#")`. A value that is not available is an empty field, never 0. Times are milliseconds with 6 decimals. A `#` inside a text field is replaced by a space, so the comment character cannot cut a row.

## Metadata header

Each CSV starts with `# file: <name>`, then these keys in this order. The key list is the same in every file and every run; a value that does not apply is empty.

| Key | Source |
|---|---|
| `format`, `format_version` | constants |
| `run_id`, `experiment_id`, `variant`, `repetition` | caller (manual export: own run id, empty experiment id, `manual`, 1) |
| `timestamp_local`, `timestamp_utc` | system clock at Start |
| `pwg_commit`, `pwg_dirty` | `git rev-parse HEAD` and `git status --porcelain --untracked-files=no`, read at build time (`cmake/pwgBuildInfo.cmake`). `pwg_dirty` counts tracked files only; empty if git was not available. |
| `build_type`, `ndebug`, `compiler` | CMake `$<CONFIG>`, `NDEBUG`, `_MSC_FULL_VER` |
| `instrumentation_compiled` | `PWG_DEBUG_VIEW_INSTRUMENTATION` |
| `gl_vendor`, `gl_renderer`, `gl_version`, `glsl_version` | `glGetString`, read once after the GL context is created |
| `query_bits_time_elapsed`, `query_bits_timestamp` | `glGetQueryiv(…, GL_QUERY_COUNTER_BITS)`, same time |
| `nvx_memory_info` | `GL_NVX_gpu_memory_info` in the extension list |
| `cpu_name`, `logical_cores`, `omp_max_threads`, `ram_total_mb`, `os_version` | CPUID brand string, `hardware_concurrency`, `omp_get_max_threads`, `GlobalMemoryStatusEx`, `RtlGetVersion`; read once at start-up |
| `ac_power`, `battery_percent`, `power_scheme`, `power_scheme_guid` | `GetSystemPowerStatus`, `PowerGetActiveScheme` + `PowerReadFriendlyName`; read at Start |
| `steady_clock_period_ns` | `std::chrono::steady_clock::period` |
| `debug_view_enabled`, `benchmark`, `render_width`, `render_height`, `frame_limit`, `swap_interval` | Debug View state at Start. `render_*` only in benchmark mode; otherwise the render size follows the Main View window. |
| `gpu_ring_frames`, `detailed_shadow_timing`, `wallclock_bracket`, `drain_timeout_frames` | profiler settings (`wallclock_bracket` is true only when a runner config sets `benchmark.wallclock_bracket`, see `debug_view_runner.md`) |
| `project_path`, `project_sha256`, `pwgsim_sha256`, `project_unsaved_changes` | open project file and `.pwgsim` sidecar on disk. With unsaved changes the hashes describe the saved files, not the editor state. |
| `world_size`, `chunk_count`, `chunk_size`, `fullscreen_main_view` | `ChunkManager`, `EditorScene` |
| `camera_path`, `camera_path_sha256` | set while a camera path is played (D-10): file of the path (`(recorded, not saved)` for a new recording) and SHA-256 of its content; empty otherwise |
| `param.<name>`, `manual.<name>` | variant parameters and manual values from the runner config (D-11) |

## frames.csv

| Column | Unit | Meaning |
|---|---|---|
| `run_id`, `variant`, `repetition` | | identifiers |
| `frame` | | frame in the window, from 0 |
| `engine_frame` | | engine frame index (matches `engine_frame` in events.csv) |
| `path_frame` | | camera path frame whose pose was applied in this frame (`CameraPath::GetAppliedFrame()`, read at the end of the frame, D-11), empty while no path plays. An existing column that was empty until D-11 is now filled; `format_version` stays 1 |
| `t_ms` | ms | frame begin minus the begin of frame 0 (steady_clock) |
| `frame_ms` | ms | begin to next begin (includes swap and limiter sleep) |
| `cpu_update_ms`, `cpu_render_ms`, `cpu_swap_ms`, `cpu_sleep_ms` | ms | CPU phases (design 5.1) |
| `gpu_frame_ms` | ms | TIMESTAMP span over scene and ImGui rendering |
| `gpu_shadow_ms`, `gpu_terrain_ms`, `gpu_vegetation_ms`, `gpu_water_ms`, `gpu_post_ms`, `gpu_imgui_ms` | ms | TIME_ELAPSED pass timers |
| `gpu_shadow_c0_ms` … `gpu_shadow_c2_ms` | ms | per-cascade TIMESTAMP spans, only with "Time each shadow cascade" |
| `draw_calls_total`, `draw_calls_shadow`, `draw_calls_main` | 1 | total = all counter slots; main = terrain + vegetation + water |
| `draw_calls_terrain`, `_vegetation`, `_water`, `_post`, `_other` | 1 | per counter slot (terrain includes the skybox, `other` = draws outside any pass) |
| `tris_total`, `tris_shadow`, `tris_main`, `tris_terrain`, `tris_vegetation`, `tris_water`, `tris_post`, `tris_other` | 1 | submitted triangles, same slots |
| `chunks_visible`, `chunks_lod0` … `chunks_lod4`, `chunks_culled` | 1 | main pass |
| `veg_visible`, `veg_culled`, `veg_shadow` | 1 | vegetation instances, sums over models |
| `gpu_dropped` | 1 | GPU samples of this frame not ready when their ring slot was reused |

GPU columns are empty when the Debug View is off, the pass did not run, or the sample was dropped. Counter columns are empty for frames that were not counted from begin to end.

## events.csv

| Column | Meaning |
|---|---|
| `run_id`, `variant`, `repetition` | identifiers |
| `frame` | window frame in which the event started (`engine_frame` − first window frame) |
| `engine_frame` | engine frame index |
| `event` | event name (table below) |
| `seq` | event sequence number; all rows of one event share it, and it gives the order in which events started |
| `group` | graph run or hydrology run the event belongs to, empty if none |
| `index` | graph node: execution order in its run, from 0; hydrology stage: stage number (0 = resampling); erosion events: pipeline step index |
| `detail` | `key=value` pairs separated by `;`: `node_id`, `node_type`, `node_enabled`, `resolution`, `solver_step`, `stage_name`, `success`, `refresh`, `talus`, `stable` |
| `metric` | `cpu_ms`, `gpu_ms`, `active_particles`, `latency_frames`, `solver_step` (thermal_stable), `occurred` (event without another value) |
| `value` | number; empty for `gpu_ms` whose result had not arrived when the file was written |
| `unit` | `ms`, `1`, `frame` |
| `source` | `steady_clock` for `cpu_ms`; `time_elapsed` or `timestamp_pair` for `gpu_ms` |

Only events that started inside the window are written. Event names are those of D-03 (`GetSimEventName`). The design (7.3, section 9) used provisional names; they map as follows:

| Design name | events.csv name |
|---|---|
| `graph_total` | `graph_execute` (CPU scope and GPU TIMESTAMP span of `ExecuteUniversalGraph`); `graph_run` is the wall time of `RunUniversalGraph` including the synchronous readback |
| `graph_node`, `graph_previews`, `height_readback_sync` | same |
| `height_readback_async` | `height_readback_queue` and `height_readback_publish` |
| `hydraulic_step`, `hydraulic_refresh`, `thermal_step`, `thermal_stable`, `hydrology_stage` | same; plus `graph_mask_pass`, `thermal_refresh`, `thermal_check` |
| `hydrology_total` | `hydrology_worker`; `hydrology_end_to_end` is Start to result on the main thread |
| `hydrology_main_read`, `hydrology_main_upload` | `hydrology_readback`, `hydrology_upload` |
| `memory_snapshot` | D-04 (not yet produced) |
| `stepping_window` | produced by the runner (D-11) for fixed-step hydraulic and thermal stepping: `metric` `steps` (unit `1`, source `runner`) and `wall_ms` (unit `ms`, source `steady_clock`), `detail` = `kind=hydraulic\|thermal;per_frame=<n>;frames=<n>;glfinish=<bool>`. One pair of rows per `hydraulic` or `thermal` prepare entry, only when the window records the prepare phase (`record: prepare`) |

## summary.csv

One row per metric that has at least one value in the window:

`run_id, variant, repetition, source, metric, n, missing, mean, median, p1, p5, p95, p99, min, max, sd, dropped`, then one `param.<name>` column per variant parameter.

- `source` = `frames` (metric = frames.csv column) or `events` (metric = `<event>.<metric>`, only metrics in ms).
- Computed from the values as written to the CSV (6 decimals), so a recomputation from frames.csv and events.csv gives the same values except for the last digit of floating-point rounding.
- `n`: values present; `missing`: rows of the window without a value.
- Percentiles: nearest rank, rank = ⌈p·n/100⌉ in integer arithmetic, clamped to [1, n], on the sorted values. `median` = p50.
- `sd`: sample standard deviation (n − 1); empty for n < 2.
- `dropped`: for GPU frame metrics, the GPU samples dropped in the whole window (all passes and spans together, the sum of `gpu_dropped`); for `<event>.gpu_ms`, the events whose GPU result had not arrived at export; 0 otherwise.

## run.yaml

```yaml
metadata: { ...the header keys above... }
result:
  status: complete
  stop_reason: user | frame_count | frame_limit
  window_first_engine_frame, window_last_engine_frame, frames, frames_missing,
  events, event_rows, events_pending_at_export, event_limit_reached,
  gpu_dropped_total, summary_rows, drain_frames, csv_write_ms, warnings
files: [frames.csv, events.csv, summary.csv]
```

`csv_write_ms` is the time spent writing the three CSV files (after the window).

A run folder written by the experiment runner (D-11) has one more top-level map, `experiment`, appended after `files`: `status` (`complete`, `failed` or `aborted`), `failures`, `warnings`, `inputs` (everything that defines the run: config hash, project, variant, parameters, benchmark settings, prepare entries, camera path and its hash, frame counts; identical for repeated runs of the same config and variant) and `results` (prepare entry log, captures with the SHA-256 of every mask buffer and screenshot, the window summary). See `debug_view_runner.md`.

## Compatibility rules

- Readers select columns by name and keys by name.
- New frames.csv columns are appended at the end, new metadata keys are appended to the key list, and new event names, metrics, units and sources may appear in events.csv. None of these changes `format_version`.
- Renaming or removing a column, key or event name, or changing a unit or the meaning of a value, increments `format_version`.
- D-04 (memory) adds `memory_snapshot` rows to events.csv (`metric` = `bytes`, `unit` = `B`, `source` = `own_accounting`, `os_process`, `driver_nvx`), through `EventRow`, and may append memory columns to frames.csv. Both are additions under these rules.

## Screenshot export (D-07)

Format `pwg-screenshot`, `format_version` 1. Code: `src/debug/exporters.*` (screenshot part), Capture tab in `src/debug/debugViewPanel.cpp`, capture point in `EditorScene::Draw`.
Design: thesis repository, `docs/plan/debug_view_design.md` section 7.5.

### Taking a screenshot

Debug window → Capture tab.

1. Set the output folder (default `captures`, relative to the working directory, `build/` when PWG is started as documented) and, if wanted, a tag.
2. Press Screenshot. The image is taken in the next frame, right after the post-process pass and before ImGui renders, so it holds the 3D scene only (no UI, no gizmos).
3. The tab shows the file path, the size and the readback and PNG write times of the last screenshot.

The button is disabled while a CSV window is recorded: the readback is synchronous and must not fall inside a measurement window (design principle 6). For the same reason `StartRecording` is refused while a screenshot is still pending. The runner (D-11) requests screenshots between windows through `DebugView::RequestScreenshot(ScreenshotRequest)` with its run folder as root and its own `RunInfo`.

The export runs in every build (with or without `PWG_DEBUG_VIEW_INSTRUMENTATION`, Debug View on or off).

### Files

`<output folder>/screenshots/<project>_<camera>_<yyyyMMdd-HHmmss>[_<tag>].png` and a `.yaml` sidecar with the same name. For a run folder (D-11) the same `screenshots/` sub-folder is used (design 7.1).

- `<project>`: stem of the open project file, `untitled` without one.
- `<camera>`: `editor`, `flyover` or `player` (active camera).
- `<yyyyMMdd-HHmmss>`: local time of the capture.
- `<tag>`: optional label (Capture tab or runner, e.g. `pathframe0450`).
- In the name parts only ASCII letters, digits, `.` and `-` are kept; any other run of characters (including `_`, the field separator) becomes one `-`.
- If a file with the same name exists (two screenshots in the same second), `-2`, `-3`, … is added after the time, before the tag. Nothing is overwritten.

### PNG

- Source: the LDR colour texture of the Main View framebuffer (`EditorScene::m_frameBuffer`, `GL_RGBA8`) after `PostProcessor::Render`.
- Size: the current scene render size, which is the Main View size or, in benchmark mode, the fixed render size.
- Pixels: read with `glGetTextureImage` as RGBA8, flipped vertically, so the first PNG row is the top of the image as shown on screen. 8 bits per channel, 4 channels, alpha 255.
- Colour: the values as displayed. `postprocess.frag` applies tone mapping and gamma 1/2.2; `GL_FRAMEBUFFER_SRGB` is off and the texture is not sRGB, so no conversion happens between the framebuffer and the screen. No colour profile chunk is written.

### Sidecar `<name>.yaml`

| Key | Meaning |
|---|---|
| `format`, `format_version` | `pwg-screenshot`, 1 |
| `file`, `tag` | PNG file name; tag or null |
| `timestamp_local` | capture time (same instant as the file name and `metadata.timestamp_local`) |
| `engine_frame` | Debug View frame index of the captured frame |
| `width`, `height`, `channels`, `pixel_format`, `row_order` | image layout (`rgba8`, `top_to_bottom`) |
| `source`, `color_encoding` | `main_view_post_processed`, `display_gamma_2.2` |
| `pixels_sha256` | SHA-256 of the RGBA rows as written to the PNG, top row first. Used for per-pixel comparisons (design 6, step 1) without depending on the PNG encoder. |
| `debug_mode`, `wireframe` | terrain debug view (Viewport tab): `none` or the selected entry name; wireframe switch |
| `camera` | pose of the camera that rendered the frame, below |
| `timing.readback_ms`, `timing.png_write_ms` | readback; flip, hash and PNG encode and write (steady_clock) |
| `metadata` | the metadata header of section "Metadata header" (D-09) at capture time, run identifiers from the caller (manual: empty `run_id`, variant `manual`) |

`camera`: `type`; `position`, `forward`, `up` (world space, from the inverse view matrix, so `position` is the eye the frame was rendered from); `yaw_deg`, `pitch_deg`, `roll_deg`; `fov_deg` (vertical), `near`, `far`, `aspect`; `view_matrix` and `projection_matrix` (16 values, column-major as in GL). Floats are written with 9 significant digits, enough to read back the same float32.

Angles follow `FlyoverCamera`: `forward = (cos p · cos y, sin p, cos p · sin y)`, so `yaw_deg = atan2(forward.z, forward.x)` in (−180, 180] and `pitch_deg = asin(forward.y)`; `roll_deg` is the rotation of `up` towards `right = normalize(cross(forward, (0, 1, 0)))`, 0 for the editor camera. All three are derived from the matrices for every camera type. For the flyover camera they equal its yaw and pitch properties up to float rounding and modulo 360° for yaw.

The compatibility rules above apply to the sidecar keys in the same way (new keys may be appended; renaming, removing or changing the meaning of a key increments `format_version`).

## Mask and heightmap export (D-08)

Format `pwg-mask`, `format_version` 1. Code: `src/debug/exporters.*` (mask part), Capture tab in `src/debug/debugViewPanel.cpp`, `DebugView::ExportMasks`.
Design: thesis repository, `docs/plan/debug_view_design.md` section 7.4.

### Exporting masks

Debug window → Capture tab → Mask export.

1. Set the output folder (shared with the screenshot part, default `captures`).
2. Tick the masks in the list. It shows every entry of `ChunkManager::GetMasks()` (engine masks, the graph's temporary masks `node_<id>_<port>` and the 256 × 256 graph editor previews `preview_node_<id>_<port>`), sorted by name, with their size. `FinalHeight` is ticked by default. The filter field narrows the list; "Tick shown" ticks every mask the filter shows.
3. Optionally type a tag (e.g. `after_thermal`) and switch the PNG preview off.
4. Press "Export ticked masks". The export runs at once and blocks the frame (about 1 s per 3999 × 3999 mask with preview, see below). The table below the button shows size, min, max, the round-trip result and the times of each mask, or the error.

The button is disabled while a CSV window is recorded: the readback and the file writes are synchronous and must not fall inside a measurement window (design principle 6). The export does not queue anything, so it never runs inside a later window either, and `StartRecording` needs no extra check for it.

The runner (D-11) calls `DebugView::ExportMasks(ChunkManager&, MaskExportRequest{root, names, tag, run, writePreview})` between windows, with its run folder as `root` and its own `RunInfo`. It returns true only if every mask was written and its round trip was identical; `GetExporters().GetLastMaskExport()` holds one `MaskExportResult` per requested name (paths, size, min/max, SHA-256, source node, times, error). The call must come from the main thread with the main GL context current.

The export runs in every build (with or without `PWG_DEBUG_VIEW_INSTRUMENTATION`, Debug View on or off). Nothing of it runs per frame: without a button press or an `ExportMasks` call it costs nothing outside the Capture tab drawing.

### Files

`<output folder>/masks/<name>[_<tag>].f32`, `.yaml` and `.png` (design 7.1 and 7.4). A run folder (D-11) uses the same `masks/` sub-folder.

- `<name>`: the mask key with the same character rule as screenshots (ASCII letters, digits, `.`, `-` kept; any other run becomes one `-`), so `node_5_height` becomes `node-5-height`. The sidecar keeps the real key in `name`.
- `<tag>`: optional, same rule (`after_thermal` becomes `after-thermal`).
- If any of the three files with that stem exists, `-2`, `-3`, … is appended to the stem. Nothing is overwritten.

### `<name>.f32` (raw data)

- float32, IEEE 754, little-endian, no header. Size = `width × height × channels × 4` bytes (`byte_size`).
- Row-major. The first row is texel row y = 0 of the GL texture (GL's bottom row, `row_order: bottom_to_top`); no flip. Value `(x, y, c)` is at index `(y · width + x) · channels + c`.
- Multi-channel masks are interleaved in channel order (R, G, B, A). Masks stored as 8 or 16 bit on the GPU (e.g. `NormalMask`, `RGBA16F`) are converted to float32 by GL; `gl_internal_format` names the stored format.
- Data: `glGetTextureImage(texture, 0, <RED|RG|RGB|RGBA>, GL_FLOAT)` into a `std::vector<float>`, with tight packing (`GL_PACK_ALIGNMENT` 4, no row length, skips or byte swap; the previous pack state and pixel pack buffer binding are restored), after `glMemoryBarrier` for image stores; written with one binary `write`.
- Size and format are read from the GL texture (`glGetTextureLevelParameteriv`), not from the `Texture` object.
- For `FinalHeight` the file is byte-identical to `Texture::ReadFloatData()` (checked in all three build configurations), so its FNV-1a equals the hashes of the D-03/D-07/D-09 checks.

Reading in Python (D-12): `numpy.fromfile(path, dtype="<f4").reshape(height, width, channels)`, row 0 = GL row 0. `numpy.flipud` gives image orientation (top row first).

Round trip (acceptance, design 7.4): after writing, the exporter reads the file back and compares it byte for byte with the exported buffer. A difference fails the export (`roundtrip_identical: true` is written only after the check).

### `<name>.png` (preview)

- 8 bit per channel. One channel: grayscale. Two channels: RGB with R, G = the channels and B = 0. Three or four channels: RGB (alpha dropped).
- Each channel is scaled with its own minimum and maximum: `floor((v − min_c) · (255 / (max_c − min_c)) + 0.5)`, computed in double. For one channel `min_c`, `max_c` are `min`, `max`. A constant channel, and NaN or infinite values, give 0.
- Flipped vertically: the first PNG row is texel row `height − 1`, so texel row 0 is the bottom row of the image.
- Only for viewing and figures; comparisons use the `.f32` files.

### Sidecar `<name>.yaml`

Keys of design 7.4, in its order:

| Key | Meaning |
|---|---|
| `format`, `format_version` | `pwg-mask`, 1 |
| `name`, `file` | mask key in `ChunkManager::GetMasks()`; `.f32` file name |
| `dtype`, `endianness` | `float32`, `little` |
| `width`, `height`, `channels`, `row_order`, `byte_size` | layout above; `row_order` is `bottom_to_top` |
| `min`, `max` | over all finite values of all channels of the exported data (9 significant digits, reads back as the same float32); null if there is none |
| `source_node` | `{id, type, port}` of the graph node output that produced the mask, or nulls; see below |
| `seeds` | `{<node id>: <seed>}` of every enabled node in the graph asset that has a `seed` parameter |
| `generation_revision` | `ChunkManager::GetGenerationRevision()`: incremented by every graph run, erosion step and hydrology upload |
| `simulation` | `hydraulic_steps`, `thermal_steps`: steps since the erosion state was last reset (graph run, reset or restore), from `TerrainComputePipeline`; `hydrology_generated`; `hydrology_running` (true means the worker was still running, so hydrology masks may be partial) |
| `project_path`, `pwg_commit` | from the metadata header (null without a saved project) |

Additions:

| Key | Meaning |
|---|---|
| `sha256` | SHA-256 of the exported buffer, equal to the SHA-256 of the `.f32` file (DEC-13 Q6: the `.f32` stays out of git, its hash is kept in the sidecar) |
| `roundtrip_identical` | result of the read-back comparison (always true in a written sidecar) |
| `non_finite_count` | NaN and infinite values (not counted in min/max) |
| `channel_min`, `channel_max` | per channel, used for the preview |
| `gl_internal_format` | storage format on the GPU, e.g. `R32F`, `RG32F`, `RGBA16F` |
| `aliases` | other keys of `GetMasks()` that share the same texture (e.g. `FinalHeight` and the erosion nodes' `node_<id>_height`) |
| `tag`, `timestamp_local`, `engine_frame` | tag or null; export time; Debug View frame index |
| `preview` | `file` (or null), `channels`, `normalization: per_channel_min_max`, `row_order: top_to_bottom` |
| `timing` | `readback_ms` (`glGetTextureImage`), `raw_write_ms` (statistics, hash and write), `verify_ms` (read-back comparison), `png_write_ms` (scale, flip, encode, write); steady_clock |
| `metadata` | the metadata header (D-09) at export time, run identifiers from the caller |

`source_node` is resolved from the current graph asset:

1. a temporary mask `node_<id>_<port>` names its node and output port;
2. `FinalHeight` and the target of an `outputMask` node: the node and port linked into the output node's input (the last enabled output node in execution order, as `ExecuteUniversalGraph` copies them); after erosion steps this is still the node linked into `outputHeight`;
3. otherwise a temporary mask that shares the texture, the last in execution order (e.g. `RiverChannelMask` → the `drainageNetwork` node, port `rivers`);
4. otherwise null: engine masks no graph node writes (`BaseHeight`, `NormalMask`, `SlopeMask`, the legacy pipeline masks) and the graph editor previews.

The compatibility rules above apply to the sidecar keys in the same way.

### Measured cost (this laptop, 2026-11-17, functional check)

FinalHeight, 3999 × 3999 × 1 (63 968 004 bytes), 5 exports per configuration, median ms (enabled / compiled in and disabled / compiled out): readback 18.1 / 19.0 / 19.0; raw write 113.3 / 106.9 / 103.8; read-back check 283.6 / 754.5 / 289.7; PNG 593.9 / 615.9 / 607.5; whole call including metadata collection 1028 / 1498 / 1027. The read-back check varied between runs (282 to 806 ms; cause not investigated); it is the same code in all configurations. These are functional numbers, not thesis data.

## Camera path record and playback (D-10)

Format `pwg-camera-path`, `format_version` 1. Code: `src/debug/cameraPath.*` (`CameraPath`, owned by `DebugView::GetCameraPath()`), `FlyoverCamera::SetPose` and the pose getters, `EditorScene::UpdateCameraPathPlayback` / `RecordCameraPathSample` (`scene/editorScene.cpp`), Capture tab in `src/debug/debugViewPanel.cpp`.
Design: thesis repository, `docs/plan/debug_view_design.md` section 7.6.

The camera path works in every build (with or without `PWG_DEBUG_VIEW_INSTRUMENTATION`, Debug View on or off) and while it is idle it costs three state tests per frame (two in `EditorScene::Update`, one in `EditorScene::Draw`).

### Recording and playing a path

Debug window → Capture tab → "Camera path (flyover camera)".

1. Set the output folder (shared with the screenshot and mask parts, default `captures`) and a name. The path file is `<folder>/paths/<name>.pwgpath.yaml`.
2. Record. The recording samples the flyover camera once per frame, but only while it is the active camera: press F2 and fly. It waits for the flyover camera if another one is active. Press F1 (editor camera) to end it, which also frees the mouse (the flyover camera captures it), or press F3; Stop recording does the same when the mouse is free. Starting a recording drops the previous path in memory.
3. Save writes the file. It refuses to replace an existing file unless "Overwrite" is ticked. Load reads `<folder>/paths/<name>.pwgpath.yaml`; the current path is replaced only if the whole file is valid, otherwise the tab shows the reason.
4. Play needs at least two keys. It switches to the flyover camera, releases the mouse so that Stop playback can be clicked, and plays the path once. "Steps per second" sets the time step (step = 1 / value, default 60 = 1/60 s). Switching to another camera (F1, F3) ends the playback. After the last frame the camera stays at the last pose and the flyover camera captures the mouse again (F1 releases it).
5. "Matrix dump" (before Play) writes the matrices of the playback to `<folder>/paths/<name>_<yyyyMMdd-HHmmss>.matrices.txt` when it ends and shows the SHA-256 of that file. Leave it off for measured runs: the rows are kept in memory until the playback ends, there is no file I/O inside it.

Only the flyover camera is recorded and played. Playback uses the fov, near and far plane stored in the file; the camera's own values are put back when it ends. The aspect ratio is the one of the render size (benchmark size or Main View size), so equal dumps need the same render size.

Every pitch from −89° to 89° can be recorded, played and loaded, including exactly 0° (the range the mouse can reach; `FlyoverCamera::SetPose` clamps to it, a file with a larger pitch is refused). E-11 needs low pitch: D-05 found that terrain culling culls nothing at the default pitch of −20° and culls most chunks at 0° (see `debug_view_controls.md`).

### `<name>.pwgpath.yaml`

```yaml
format: pwg-camera-path
format_version: 1
camera: flyover
fov_deg: 60
near: 0.00999999978
far: 20000
duration_s: 0.690559089
keys:            # t, px, py, pz, yaw_deg, pitch_deg, roll_rad
  - [0, 899.774963, 699.825012, -899.774963, -45, -20, 0]
  - [0.00331760012, 8.9986496, 61.6288109, -0.134989873, -199.100006, -20, 0.00488782721]
```

| Key | Meaning |
|---|---|
| `format`, `format_version`, `camera` | `pwg-camera-path`, 1, `flyover` (any other value is refused) |
| `fov_deg`, `near`, `far` | vertical field of view in degrees (0 < fov < 180), clip planes (0 < near < far); the values of the flyover camera at the first sample |
| `duration_s` | time of the last key; must equal it |
| `keys` | at least two rows `[t, px, py, pz, yaw_deg, pitch_deg, roll_rad]`: seconds since the first sample (first row 0, strictly increasing), world position, yaw and pitch in degrees, gimbal roll in radians |

Floats have 9 significant digits, which reads back the same float32 (checked: a saved path loads with keys bit-identical to the recording). Yaw is unwrapped while recording (each key takes the representation of the angle closest to the previous key), so interpolation never jumps by 360°. Angles follow `FlyoverCamera` (see the screenshot sidecar above). A path file is valid if every number is finite, pitch is in [−89, 89] and the rules in the table hold; the SHA-256 of the file bytes is `camera_path_sha256` in the metadata of a run that plays it.

### Playback

Playback is indexed by frame, not by wall time (design 7.6). Path frame k has `t = k · step`; its pose is the linear interpolation of the two keys around `t` (position, yaw, pitch and roll component-wise, in double precision, then rounded to float). The path has `floor(duration / step) + 1` frames. `EditorScene::Update` sets the pose with `FlyoverCamera::SetPose` once per frame and does not call the camera update in that frame, so the pose never depends on input, `dt` or the frame rate.

### Matrix dump (`*.matrices.txt`)

Text, `\n` line ends, no time stamps and no file names, so two dumps of the same playback are byte-identical. Header lines start with `# `: `format`, `format_version`, `path_sha256`, `key_count`, `step_s_hex`, `render_size`, `columns`. Then one line per path frame: `path_frame`, 16 values of the view matrix and 16 values of the projection matrix, separated by blanks, column-major as in GL, as hex floats (`printf("%a", (double)value)`). The matrices are those the scene is rendered with: they are taken in `EditorScene::Draw` after the camera got the final aspect ratio of the frame. Acceptance (D-10): two playbacks of the same path at the same render size give identical dumps.

### Interface for the runner (D-11)

`DebugView::GetCameraPath()`: `Load(file, error)`, `StartPlayback({stepSeconds, matrixDumpFile}, error)`, `IsPlaying()`, `GetAppliedFrame()` (path frame of the pose applied in the current frame, −1 if none), `GetFrameCount()`, `GetFile()`, `GetSha256()`, `StopPlayback()`. While a path plays, `DebugView::CollectRunMetadata` sets `camera_path` and `camera_path_sha256`. The runner measures a fixed number of path frames and copies the path file to the run folder's `paths/` (design 7.1); it plays the copy. The `path_frame` column of frames.csv is filled by `DebugView::EndFrame` from `GetAppliedFrame()` (D-11).

### Measured cost (this laptop, 2026-11-20, functional check)

Idle (no path loaded or playing): `EditorScene::Update` makes two calls (`UpdateCameraPathPlayback`, `RecordCameraPathSample`) and `EditorScene::Draw` one state test. Timed in place with `steady_clock` over 1540 frames per block, including the clock reads of the measurement: with the calls median 200 ns, mean 152 to 262 ns (Debug View enabled, disabled, instrumentation compiled out, four blocks each); with the calls compiled out of the harness build (only the clock reads left) mean 54 to 61 ns, median 0 to 100 ns. The clock tick is 100 ns, so the added cost is about 0.1 to 0.2 µs per frame (mean), at the resolution limit. Frame time, benchmark mode 1280 × 720, 300 warm-up and 1500 measured frames per block, Debug View enabled, two interleaved runs each, medians of `frame_ms` in ms for the blocks (default pose, low pose, low pose again, default pose again): without the calls 3.33 / 3.37 / 3.52 / 3.47 and 3.47 / 3.84 / 3.99 / 3.55; with them 3.40 / 3.63 / 3.80 / 3.49 and 3.50 / 4.06 / 4.08 / 3.51. The two runs without the calls differ from each other by up to 0.47 ms, more than the runs differ from each other with and without the calls, so no difference is measurable at this noise level; this does not prove zero cost.

Not timed: the cost of a frame while a path is recorded or played (one `push_back` of a 32-byte key; a binary search, a linear interpolation, `SetPose` and, with the matrix dump, one string append of about 700 bytes per frame), and the time of Save, Load and the dump write. The dump of 41 frames is 28 KB.
