# Debug View performance export (D-09)

Format `pwg-debugview-csv`, `format_version` 1. Code: `src/debug/csvWriter.*`, `src/debug/runMetadata.*`.
Design: thesis repository, `docs/plan/debug_view_design.md` sections 7.1 to 7.3.

## Recording a window

Debug window → Performance tab → CSV export.

1. Set the output folder (default `perf_exports`, relative to the working directory, which is `build/` when PWG is started as documented) and the number of frames (0 = until Stop).
2. Start recording. The window begins with the next frame. Debug View switch, benchmark mode, render size, ring frames K, cascade timing and the Simulation tab's Clear button are locked until the export is written, because the metadata header describes the whole window.
3. Stop and export (or wait for the frame count). The recorder keeps rendering until every frame of the window has its GPU results (K frames) and no GPU event that started inside the window is pending (at most 600 frames). Then it writes the files, at the start of a frame outside the window. Nothing is written to disk inside the window.

Abort drops the window. Closing the application during a recording drops it too.

The experiment runner (D-11) uses the same code through `DebugView::StartRecording(RunInfo, root, frames)` and `StopRecording()`.

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
| `gpu_ring_frames`, `detailed_shadow_timing`, `wallclock_bracket`, `drain_timeout_frames` | profiler settings (`wallclock_bracket` is always false until D-11) |
| `project_path`, `project_sha256`, `pwgsim_sha256`, `project_unsaved_changes` | open project file and `.pwgsim` sidecar on disk. With unsaved changes the hashes describe the saved files, not the editor state. |
| `world_size`, `chunk_count`, `chunk_size`, `fullscreen_main_view` | `ChunkManager`, `EditorScene` |
| `camera_path`, `camera_path_sha256` | empty until D-10 |
| `param.<name>`, `manual.<name>` | variant parameters and manual values from the runner config (D-11) |

## frames.csv

| Column | Unit | Meaning |
|---|---|---|
| `run_id`, `variant`, `repetition` | | identifiers |
| `frame` | | frame in the window, from 0 |
| `engine_frame` | | engine frame index (matches `engine_frame` in events.csv) |
| `path_frame` | | camera path frame, empty until D-10 |
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
| `stepping_window` | D-11 (not yet produced) |

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

## Compatibility rules

- Readers select columns by name and keys by name.
- New frames.csv columns are appended at the end, new metadata keys are appended to the key list, and new event names, metrics, units and sources may appear in events.csv. None of these changes `format_version`.
- Renaming or removing a column, key or event name, or changing a unit or the meaning of a value, increments `format_version`.
- D-04 (memory) adds `memory_snapshot` rows to events.csv (`metric` = `bytes`, `unit` = `B`, `source` = `own_accounting`, `os_process`, `driver_nvx`), through `EventRow`, and may append memory columns to frames.csv. Both are additions under these rules.
