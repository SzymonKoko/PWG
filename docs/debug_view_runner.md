# Debug View experiment runner (D-11)

Code: `src/debug/experimentRunner.{h,cpp}`, Experiment tab in `src/debug/debugViewPanel.cpp`, command line in `src/main.cpp`, per-frame tick in `Application::TickExperiment`.
Design: thesis repository, `docs/plan/debug_view_design.md` section 8 (protocol), 7.1 (run folder), 9 (E-01 … E-13). Formats of the files it writes: `debug_view_export.md`.

The runner loads a YAML config and, for every variant and repetition, loads the project, applies parameters, prepares, exports captures, warms up, measures and writes the CSV files. It never runs a loop of its own: `Application::Run` calls it once per frame after the frame ended. It works in every build; with `PWG_DEBUG_VIEW_INSTRUMENTATION=OFF` the CSV files hold the frame timing only (design 6).

## Running

```bash
PWG.exe --experiment <config.yaml> [--out <folder>] [--exit-when-done]
```

Start it from `build/bin/Release` (the project and the engine assets are found relative to the working directory, as for the editor). The run starts after the first frame. `--out` replaces `output:` of the config. `--exit-when-done` closes the application after the last unit; the exit code is 0 if every unit was complete and 1 if the config was rejected, the run was aborted or a unit had problems (2 for a wrong command line).

A run can also be started from the Debug window, tab Experiment: config file, optional output folder, Start, Abort, and the state (unit, variant, repetition, phase, frame counter, run folder). In the fullscreen main view the Debug window is not drawn; **F9 aborts the run** there too.

Examples: `examples/experiments/frames_example.yaml` (LOD on and off along a camera path, two repetitions) and `examples/experiments/events_example.yaml` (graph, fixed-step erosion and hydrology timing). They write to `build/experiment-runs/<id>/`. The project `runner-test.pwg` next to them is a 9 × 65 world with the full pipeline (noise, particle and thermal erosion, drainage network, grass and a tree layer); the camera path `paths/orbit_example.pwgpath.yaml` is a hand-written 2 s orbit, not a recording.

## Config

Unknown keys, unknown parameters and wrong value types reject the config before anything runs (the reason goes to the log; exit code 1). Relative paths are relative to the config file.

```yaml
format: pwg-experiment            # required
format_version: 1                 # required
experiment_id: E-10               # required, used as a folder and file name
description: "..."
project: ../projects/x.pwg        # required
expect_renderer: "RTX 3060"       # optional: the run stops before measuring if GL_RENDERER does not contain it
output: ../results/E-10           # default: <config folder>/results/<experiment_id>; --out replaces it
debug_view: true                  # false = compiled in but switched off (overhead check); ignored when compiled out
benchmark:
  enabled: true                   # false: no benchmark mode, no cpu_sleep_ms check
  render_size: [1920, 1080]
  fullscreen_main_view: false     # true: fullscreen window during warm-up and measurement
  gpu_ring_frames: 4
  wallclock_bracket: false        # glFinish before and after each fixed-step window; written to the metadata
  max_gpu_dropped: 0              # more dropped GPU samples in a window: the run is marked failed
manual: { acer_mode: "...", nvidia_driver: "...", driver_vsync_override: "..." }   # copied to manual.* in the metadata
params: { ... }                   # parameters applied to every variant (below)
prepare: [ ... ]                  # entries run once per run, in order (below)
camera: { path: ..., dt: 0.0166666667, loop: false, matrix_dump: false }
warmup: { frames: 300 }
measure: { frames: 1800 }         # path frames; 0 or absent: no measurement window
record: measure                   # measure | prepare | none, see "CSV window"
repetitions: 5
order: interleaved                # or sequential
cooldown_s: 30
variants: ...                     # list or matrix (below)
captures: [ ... ]                 # below
```

### Parameters

The runner sets only these keys; each maps to an existing setter. They are applied after the project was loaded, in the order terrain, `graph.file`, graph nodes, the rest. Runtime controls that are not part of a project (`lod.*`, `culling.enabled`, `vegetation.enabled`) go back to their defaults at the start of every unit, so no state carries over.

| Key | Value | Effect |
|---|---|---|
| `terrain.chunk_count`, `terrain.chunk_size` | integer 3 … 31, 33 … 257 | `ChunkManager::ConfigureTerrain` and `GenerateWorld` (as `ProjectSerializer::Load` does); the graph runs in `prepare` |
| `graph.file` | project file | replaces the graph by the `terrainGraphV2` of another project file (`ProjectSerializer::LoadGraph`, same reader and migrations as `Load`) |
| `graph.node.<id>.<param>` | number, integer, bool or text by the parameter type | any graph node parameter (`seed`, `octaves`, `amount`, `talusAngle`, `density`, `renderDistance`, …); the node and the parameter must exist |
| `graph.node.<id>.enabled` | bool | enable or bypass a node |
| `lod.enabled`, `lod.scale`, `lod.distances` | bool, number > 0 and up to 100, four numbers not decreasing | D-05 setters (`debug_view_controls.md`) |
| `culling.enabled` | bool | terrain chunk culling (D-05) |
| `vegetation.enabled` | bool | skips the vegetation draw and its shadow draw (`EditorScene::SetVegetationEnabled`) |
| `shadows.enabled` | bool | `EnvironmentSettings::shadowsEnabled` |
| `water.visible` | bool | `HydraulicSimulationSettings::showWaterSurface` |

`terrain.*` and `graph.*` parameters take effect only when the graph runs again, so a config that sets one needs `run_graph` (or `refresh_assets` for vegetation layers) in `prepare`; otherwise it is rejected. A parameter set in `params` and again in a variant takes the variant's value.

### Variants

```yaml
variants:                         # explicit list
  - { name: lod_on,  params: { lod.enabled: true } }
  - { name: lod_off, params: { lod.enabled: false } }
```
```yaml
variants:                         # matrix: every combination, the first key changes slowest
  matrix:
    lod.enabled: [true, false]
    lod.scale: [0.5, 1.0, 2.0]
```

A matrix variant is named `lod.enabled=true;lod.scale=0.5`. Without `variants` there is one variant, `default`. The variant name is the `variant` column of the CSV files, and every parameter is a `param.<name>` column of summary.csv.

### Prepare entries

Each entry has one action and an optional `name:` (default: the action, `hydraulic_2` for the second use). Captures refer to the name.

| Action | Effect |
|---|---|
| `run_graph: true` | `ChunkManager::RunUniversalGraph` |
| `refresh_assets: true` | `ChunkManager::RefreshProceduralAssets` (vegetation layers: `density`, `renderDistance`) |
| `hydrology: generate` | starts the worker and renders frames until the result is in (30 min limit) |
| `hydraulic: { steps: N, per_frame: K }` | fixed-step mode: `ChunkManager::StepParticleErosion` K times per frame, N steps; the dt-driven path stays paused (design F5). Needs an enabled particle erosion node |
| `thermal: { steps: N, per_frame: K }` or `{ until: stable, max_steps: M, per_frame: K }` | the same with `StepThermalErosion`; `until: stable` stops at the first stable step and fails if `max_steps` is reached first |
| `save: { file: x.pwg }` | `ProjectSerializer::Save` to `<run folder>/x.pwg` (and its `.pwgsim`). The loaded project and the editor's current project path are not touched |
| `reload: { file: x.pwg }` or `reload: project` | loads `<run folder>/x.pwg`, or the config's project again (E-04). Parameters are not applied again |
| `wait: { frames: N }` | renders N frames |

Each fixed-step entry writes one `stepping_window` event (steps and wall time from the first step to the end of the last one, including the frames rendered in between when `per_frame` is smaller than `steps`; with `per_frame` equal to `steps` it is the stepping alone) when the CSV window covers the prepare phase.

### Captures

```yaml
captures:
  - after: prepare                # prepare | measure | the name of a prepare entry
    screenshot: { path_frame: 0, tag: start }   # or `screenshot: true`
    masks: [FinalHeight]          # keys of ChunkManager::GetMasks(); `preview: false` skips the PNG
```

Captures run between windows (principle 6). `after: <entry name>` runs after that entry, which is how E-13 captures every pipeline stage; it needs `record: none` or `measure`. A screenshot with `path_frame: k` plays the copied path, requests the capture one frame earlier and takes it while the path is still playing, so the file shows exactly path frame k, and the sidecar's metadata names the path. Without `path_frame` the camera is shown as it is. Files go to `<run folder>/screenshots/` and `<run folder>/masks/` with the tag `after_<name>` for masks.

### Camera

`camera.path` is copied byte for byte to `<run folder>/paths/` and played from the copy; `camera_path` and `camera_path_sha256` in the metadata name it. Playback is indexed by frame with step `camera.dt` (D-10). Warm-up plays the path in a loop, so the scene is warm along it. The measurement restarts it, so window frame 0 shows path frame 0 and `path_frame` of frames.csv counts 0, 1, 2 …. If `measure.frames` is more than the path has frames the unit fails, unless `camera.loop: true` restarts the path. `matrix_dump: true` writes `<run folder>/paths/<name>.matrices.txt` (D-10 format; written when the playback ends, outside the window).

## What one unit does (design 8.4)

```text
load project -> apply parameters -> benchmark mode on -> [CSV window of prepare] -> prepare entries
  -> captures after entries -> [window ends, files written] -> captures after prepare
  -> warm-up -> [CSV window of measure: measure.frames frames] -> files written -> captures after measure -> report -> cooldown
```

Each step waits three frames after a load, the parameters or benchmark mode so the scene has updated. Every unit reloads the project. `interleaved` runs repetition 1 of every variant, then repetition 2; `sequential` runs all repetitions of a variant together. If a step fails (project load, parameter, graph, hydrology, capture, dropped samples above the limit, `cpu_sleep_ms` above 0 in benchmark mode, a limiter or VSync that is still active at the start of the measurement) the unit is closed as failed with the reason in its `run.yaml`, and the remaining units continue. A failed renderer check stops the run before the first measurement.

### CSV window

`record: measure` (default when `measure.frames` is above 0): the window covers exactly the measured frames, and events started in them. `record: prepare`: the window covers the prepare entries instead (graph, step and hydrology events, `stepping_window`; frames.csv holds the frames in which they ran). `record: none`: no CSV files, only captures and `run.yaml`. The recorder writes one window per folder, so one run records either the prepare phase or the measurement.

## Output

```text
<output>/
  <experiment_id>.yaml           copy of the config (a different config with the same id: <id>-<hash8>.yaml)
  <local time>_r01/              one folder per unit: run id = start time of the run and the unit number
    run.yaml                     metadata, result of the window and, appended, the `experiment` map
    frames.csv  events.csv  summary.csv
    masks/  screenshots/  paths/
    <saved copies of the project>
```

`run.yaml` ends with `experiment:` (`debug_view_export.md`). `experiment.inputs` is the same for repeated runs of the same config, variant and project; `experiment.results` holds the SHA-256 of every exported mask buffer and screenshot (pixels, top row first) and the measured times of the prepare entries.

## Reproducing a run

Running the stored config again gives the same `experiment.inputs`, the same `project_sha256`, the same hashes of masks, and new timings. Screenshots repeat as well, with one exception that is the engine's, not the runner's: vegetation (`m_time`) and water (`m_waterTime`) animate with the accumulated frame time, so a frame that shows them depends on how much time passed since the project was loaded. Use `vegetation.enabled: false` and a view without water, or compare masks, when a hash must repeat.
