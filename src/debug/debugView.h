#ifndef SRC_DEBUG_DEBUGVIEW_H_
#define SRC_DEBUG_DEBUGVIEW_H_

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>
#include "debug/cameraPath.h"
#include "debug/cpuProfiler.h"
#include "debug/csvWriter.h"
#include "debug/exporters.h"
#include "debug/frameStats.h"
#include "debug/gpuProfiler.h"
#include "debug/memoryStats.h"
#include "debug/renderStats.h"
#include "debug/runMetadata.h"
#include "debug/simulationStats.h"

// Set by CMake (option PWG_DEBUG_VIEW_INSTRUMENTATION, default ON).
#ifndef PWG_DEBUG_VIEW_INSTRUMENTATION
#define PWG_DEBUG_VIEW_INSTRUMENTATION 1
#endif

namespace pwg
{
	class Window;
	class ChunkManager;
}

namespace pwg::debug
{
	class ExperimentRunner;

	/** @brief True if CPU scopes and GPU queries are compiled in. */
	constexpr bool InstrumentationCompiled = PWG_DEBUG_VIEW_INSTRUMENTATION != 0;

	/**
	* @brief Debug View core: runtime switch, frame begin/end hooks, benchmark mode, and owner of the profilers.
	*
	* Frame timing (frame_ms and the FramePhase times) runs in every build and whether the Debug View is
	* enabled or not. CPU scopes and GPU queries run only when compiled in and enabled at runtime.
	* One instance is created by Application after the GL context exists.
	*/
	class DebugView
	{
	public:
		using Clock = std::chrono::steady_clock;

		explicit DebugView(Window& window);
		~DebugView();
		DebugView(const DebugView&) = delete;
		DebugView& operator=(const DebugView&) = delete;

		/** @brief Returns the instance. Only valid between construction and destruction (see HasInstance). */
		static DebugView& Get() { return *s_instance; }
		static bool HasInstance() { return s_instance != nullptr; }

		/** @brief Runtime switch of the measurement points. One relaxed atomic load. */
		static bool IsEnabled() { return InstrumentationCompiled && s_enabled.load(std::memory_order_relaxed); }
		void SetEnabled(bool enabled);

		/** @brief Called at the top of the main loop, before any work of the frame. */
		void BeginFrame();

		/** @brief Called at the end of the main loop, after the limiter sleep. */
		void EndFrame();

		/** @brief Must run while the main GL context is current, before glfwTerminate. */
		void Shutdown();

		/** @brief Adds CPU time of a frame phase to the current frame record. */
		void AddPhaseTime(FramePhase phase, Clock::duration duration);

		/**
		* @brief Benchmark mode: no frame limit, VSync off, fixed render resolution.
		* The previous limit and VSync state are restored when it is switched off.
		*/
		void SetBenchmarkMode(bool enabled);
		bool IsBenchmarkMode() const { return m_benchmark; }
		void SetBenchmarkRenderSize(int width, int height);
		int GetBenchmarkRenderWidth() const { return m_benchmarkWidth; }
		int GetBenchmarkRenderHeight() const { return m_benchmarkHeight; }

		/** @brief Ring size K of the GPU query ring, applied at the next frame begin. */
		void SetGpuRingFrames(uint32_t frames) { m_requestedRingFrames = frames; }

		/**
		* @brief The runner's "wall-clock bracket" option (design 5.2): glFinish before and after a stepping window.
		* Set by the runner; recorded in the metadata as `wallclock_bracket`.
		*/
		void SetWallclockBracket(bool enabled) { m_wallclockBracket = enabled; }
		bool IsWallclockBracket() const { return m_wallclockBracket; }

		/** @brief Detailed option: one TIMESTAMP span per shadow cascade inside the shadow pass timer. Off by default. */
		void SetDetailedShadowTiming(bool enabled) { m_detailedShadowTiming = enabled; }
		bool IsDetailedShadowTiming() const { return m_detailedShadowTiming; }

		uint64_t GetFrameIndex() const { return m_frameIndex; }
		CpuProfiler& GetCpuProfiler() { return m_cpu; }
		GpuProfiler& GetGpuProfiler() { return m_gpu; }
		RenderStats& GetRenderStats() { return m_render; }
		const FrameStats& GetFrameStats() const { return m_frameStats; }
		SimulationStats& GetSimulationStats() { return m_sim; }
		RunMetadata& GetRunMetadata() { return m_runMetadata; }
		const CsvRecorder& GetRecorder() const { return m_recorder; }

		/**
		* @brief Full metadata header of a run starting now (design section 7.2): run identifiers, build,
		* GL context and machine, Debug View and benchmark state, scene. Reads files for the SHA-256 values.
		*/
		Metadata CollectRunMetadata(const RunInfo& run, std::chrono::system_clock::time_point start) const;

		/**
		* @brief Starts a CSV measurement window with the next frame (D-09). The files go to
		* `outputRoot / run.runId`; an empty run id becomes `<local time>_mNN`. `frameCount` 0 records until
		* StopRecording. Returns false if a window is active or the folder exists.
		*/
		bool StartRecording(RunInfo run, const std::filesystem::path& outputRoot, uint64_t frameCount);

		/** @brief Ends the window with the current frame; the files are written once its results are in. */
		void StopRecording() { m_recorder.Stop(m_frameIndex); }

		/** @brief Drops the window without writing. */
		void AbortRecording() { m_recorder.Abort(); }

		/**
		* @brief Adds a row of the runner (`stepping_window`) to the events.csv of the window being recorded. The row takes
		* the next event sequence number and the current frame; false if no window records.
		*/
		bool AddRecordedEvent(EventRow row)
		{
			row.sequence = m_sim.ReserveSequence();
			row.engineFrame = m_frameIndex;
			return m_recorder.AddEventRow(std::move(row));
		}

		bool IsRecording() const { return m_recorder.IsActive(); }

		/**
		* @brief Queues a screenshot of the scene for the next frame (D-07). Refused while a CSV window is
		* recorded, because the readback is synchronous (design principle 6), or while another one is pending.
		*/
		bool RequestScreenshot(ScreenshotRequest request);

		/**
		* @brief Takes the pending screenshot. Called by EditorScene::Draw after post-processing, before ImGui
		* renders. Adds the run metadata of the capture time to the sidecar.
		*/
		void CaptureScreenshot(const ScreenshotSource& source);

		/**
		* @brief Exports masks of `chunks` now (D-08, design 7.4): `<root>/masks/<name>[_<tag>].f32` + `.yaml`
		* sidecar + `.png` preview. Synchronous readback and file writes, so it is refused while a CSV window is
		* recorded (design principle 6). Main GL context only. Returns true if every requested mask was written
		* and its round trip was identical; per-mask results: GetExporters().GetLastMaskExport().
		*/
		bool ExportMasks(ChunkManager& chunks, const MaskExportRequest& request);

		/** @brief Drops a pending screenshot request (the runner does it when a run ends early). */
		void CancelScreenshot() { m_exporters.CancelScreenshot(); }

		/** @brief Read access (pending request, last result). Requests go through RequestScreenshot / ExportMasks. */
		const Exporters& GetExporters() const { return m_exporters; }

		/**
		* @brief Flyover camera path (D-10, design 7.6): record, play back, save, load. EditorScene::Update drives the
		* camera from it. The runner (D-11) loads a path and starts the playback through it. Works in every build.
		*/
		CameraPath& GetCameraPath() { return m_cameraPath; }
		const CameraPath& GetCameraPath() const { return m_cameraPath; }

		/** @brief "Performance" tab of the Debug window (debugViewPanel.cpp). */
		void DrawPerformanceTab();

		/**
		* @brief "Render" tab of the Debug window: chunk and LOD controls (D-05, every build) and render counters
		* (debugViewPanel.cpp). `chunks` provides the controls; null hides them.
		*/
		void DrawRenderTab(ChunkManager* chunks);

		/** @brief "Simulation" tab of the Debug window: graph, erosion, hydrology and readback timing (debugViewPanel.cpp). */
		void DrawSimulationTab();

		/**
		* @brief "Memory" tab of the Debug window (D-04): own accounting, process RAM and driver VRAM, each with its
		* source label (debugViewPanel.cpp). Instrumented builds only.
		*/
		void DrawMemoryTab();

		/**
		* @brief Takes a memory snapshot (own accounting, process RAM, driver VRAM; needs the GL context, main thread) and
		* adds its rows (`memory_snapshot`, one shared sequence number) to the events.csv of the window being recorded.
		* `point` says where it was taken (`point=` in the detail column). False if no window records, the frame is not
		* inside it, or the instrumentation is compiled out; no query is made in that case.
		*/
		bool RecordMemorySnapshot(const std::string& point);

		/**
		* @brief Experiment runner (D-11, design 8). Application creates it and registers it here; the Experiment tab
		* and the runner's metadata use it through this pointer. Null until registered.
		*/
		void SetExperimentRunner(ExperimentRunner* runner) { m_runner = runner; }
		ExperimentRunner* GetExperimentRunner() const { return m_runner; }

		/** @brief "Experiment" tab of the Debug window: load a config, start, abort, progress (debugViewPanel.cpp). */
		void DrawExperimentTab();

		/**
		* @brief "Capture" tab of the Debug window: screenshot (D-07) and mask export (D-08) (debugViewPanel.cpp).
		* `chunks` provides the masks; null hides the mask list.
		*/
		void DrawCaptureTab(ChunkManager* chunks);

	private:
		/** @brief GPU query and CPU scope part of the Performance tab (instrumented builds only). */
		void DrawProfilerSection();

		/** @brief Chunk and LOD controls of the Render tab (every build). */
		void DrawChunkLodSection(ChunkManager* chunks);

		/** @brief Counter tables of the Render tab (instrumented builds only). */
		void DrawRenderCounters();

		/** @brief Tables of the Simulation tab (instrumented builds only). */
		void DrawSimulationTables();

		/** @brief CSV export part of the Performance tab (every build). */
		void DrawExportSection();

		/** @brief Screenshot part of the Capture tab (every build). */
		void DrawScreenshotSection();

		/** @brief Mask export part of the Capture tab (every build). */
		void DrawMaskSection(ChunkManager* chunks);

		/** @brief Camera path part of the Capture tab (every build). */
		void DrawCameraPathSection();

		static inline DebugView* s_instance{ nullptr };
		static inline std::atomic<bool> s_enabled{ false };

		Window& m_window;
		CpuProfiler m_cpu;
		GpuProfiler m_gpu;
		RenderStats m_render;
		FrameStats m_frameStats;
		SimulationStats m_sim;
		RunMetadata m_runMetadata;
		CsvRecorder m_recorder{ m_frameStats, m_sim };	/**< Declared after the stats it reads. */
		Exporters m_exporters;
		CameraPath m_cameraPath;
		std::vector<GpuEventResult> m_gpuEventResults;	/**< Reused by BeginFrame. */
		uint32_t m_manualExportCounter{ 0 };
		int64_t m_memorySnapshotIndex{ 0 };		/**< `index` of the next memory_snapshot of the window; restarts with every window. */
		MemorySnapshot m_memoryShown;			/**< Snapshot of the Memory tab, refreshed a few times per second. */
		double m_memoryShownTime{ -1.0 };		/**< ImGui time of m_memoryShown; negative = none yet. */

		uint64_t m_frameIndex{ 0 };
		bool m_frameStarted{ false };
		bool m_gpuInitialized{ false };
		uint32_t m_requestedRingFrames{ GpuProfiler::DefaultRingFrames };
		bool m_detailedShadowTiming{ false };
		bool m_wallclockBracket{ false };
		ExperimentRunner* m_runner{ nullptr };
		Clock::time_point m_frameBegin{};

		bool m_benchmark{ false };
		int m_benchmarkWidth{ 1920 };
		int m_benchmarkHeight{ 1080 };
		unsigned int m_savedFrameLimit{ 0 };
		bool m_savedVSync{ false };

		std::vector<float> m_plotScratch;		/**< Used by DrawPerformanceTab only. */
		std::vector<SimEvent> m_simScratch;		/**< Used by DrawSimulationTab only. */
		char m_exportRoot[260]{ "perf_exports" };	/**< Export folder of the Performance tab, relative to the working directory. */
		int m_exportFrames{ 0 };					/**< Frames per window in the Performance tab, 0 = until Stop. */
		char m_experimentConfig[260]{};				/**< Config file of the Experiment tab. */
		char m_experimentOut[260]{};				/**< Optional output folder of the Experiment tab (empty: the config's `output`). */
		std::string m_experimentMessage;			/**< Result of the last Start press. */
		char m_captureRoot[260]{ "captures" };		/**< Capture root of the Capture tab, relative to the working directory. */
		char m_screenshotTag[64]{};					/**< Optional tag of the next screenshot. */
		std::vector<std::string> m_maskSelection{ "FinalHeight" };	/**< Masks ticked in the Capture tab. */
		char m_maskFilter[64]{};					/**< Name filter of the mask list. */
		char m_maskTag[64]{};						/**< Optional tag of the next mask export. */
		bool m_maskPreview{ true };					/**< Write the PNG preview. */
		char m_pathName[64]{ "flyover_path" };		/**< Name of the camera path file in the Capture tab. */
		int m_pathStepsPerSecond{ 60 };				/**< Playback step as path frames per second (step = 1 / value). */
		bool m_pathMatrixDump{ false };				/**< Write the matrix dump of the next playback. */
		bool m_pathOverwrite{ false };				/**< Allow Save to replace an existing file. */
		std::string m_pathMessage;					/**< Result of the last Save / Load / Play press. */
		bool m_pathMessageIsError{ false };
	};

	/**
	* @brief Adds the elapsed steady_clock time of its lifetime to a frame phase. Active in every build.
	*/
	class FramePhaseTimer
	{
	public:
		explicit FramePhaseTimer(FramePhase phase) : m_phase(phase), m_start(DebugView::Clock::now()) {}
		~FramePhaseTimer()
		{
			if (DebugView::HasInstance())
			{
				DebugView::Get().AddPhaseTime(m_phase, DebugView::Clock::now() - m_start);
			}
		}
		FramePhaseTimer(const FramePhaseTimer&) = delete;
		FramePhaseTimer& operator=(const FramePhaseTimer&) = delete;

	private:
		FramePhase m_phase;
		DebugView::Clock::time_point m_start;
	};

	/**
	* @brief RAII CPU scope, use through PWG_CPU_SCOPE. When disabled it costs one branch.
	*/
	class CpuScope
	{
	public:
		explicit CpuScope(const char* name)
		{
			if (DebugView::IsEnabled())
			{
				CpuProfiler& profiler = DebugView::Get().GetCpuProfiler();
				m_frameIndex = profiler.GetFrameIndex();
				m_index = profiler.BeginScope(name);
			}
		}
		~CpuScope()
		{
			if (m_index >= 0 && DebugView::HasInstance())
			{
				DebugView::Get().GetCpuProfiler().EndScope(m_index, m_frameIndex);
			}
		}
		CpuScope(const CpuScope&) = delete;
		CpuScope& operator=(const CpuScope&) = delete;

	private:
		int32_t m_index{ -1 };
		uint64_t m_frameIndex{ 0 };
	};

	/**
	* @brief RAII GPU pass timer (TIME_ELAPSED), use through PWG_GPU_SCOPE. Main GL context only.
	*/
	class GpuScope
	{
	public:
		explicit GpuScope(GpuPass pass) : m_pass(pass)
		{
			if (DebugView::IsEnabled())
			{
				m_active = DebugView::Get().GetGpuProfiler().BeginPass(pass);
			}
		}
		~GpuScope()
		{
			if (m_active && DebugView::HasInstance())
			{
				DebugView::Get().GetGpuProfiler().EndPass(m_pass);
			}
		}
		GpuScope(const GpuScope&) = delete;
		GpuScope& operator=(const GpuScope&) = delete;

	private:
		GpuPass m_pass;
		bool m_active{ false };
	};

	/**
	* @brief RAII render pass, use through PWG_RENDER_PASS. Combines a CPU scope, the pass's TIME_ELAPSED
	* timer and the counter slot of the draws inside it. Its CPU time is command submission, not GPU time.
	*/
	class RenderPassScope
	{
	public:
		RenderPassScope(GpuPass pass, const char* name) : m_cpu(name), m_gpu(pass), m_pass(pass)
		{
			if (DebugView::IsEnabled())
			{
				m_previous = DebugView::Get().GetRenderStats().BeginPass(pass);
				m_start = DebugView::Clock::now();
				m_active = true;
			}
		}
		~RenderPassScope()
		{
			if (m_active && DebugView::HasInstance())
			{
				DebugView::Get().GetRenderStats().EndPass(m_pass, m_previous, DebugView::Clock::now() - m_start);
			}
		}
		RenderPassScope(const RenderPassScope&) = delete;
		RenderPassScope& operator=(const RenderPassScope&) = delete;

	private:
		CpuScope m_cpu;
		GpuScope m_gpu;
		GpuPass m_pass;
		size_t m_previous{ OtherCounterPass };
		DebugView::Clock::time_point m_start{};
		bool m_active{ false };
	};

	/**
	* @brief RAII shadow cascade inside the shadow pass, use through PWG_SHADOW_CASCADE.
	* Sets the cascade of the shadow counters and, with the detailed option on, times the cascade with a
	* TIMESTAMP pair. A TIME_ELAPSED timer cannot be used here because the shadow pass timer is active.
	*/
	class ShadowCascadeScope
	{
	public:
		explicit ShadowCascadeScope(int cascade)
		{
			if (!DebugView::IsEnabled())
			{
				return;
			}
			DebugView& view = DebugView::Get();
			view.GetRenderStats().SetCascade(cascade);
			m_counting = true;
			if (view.IsDetailedShadowTiming() && cascade >= 0 && static_cast<size_t>(cascade) < ShadowCascadeCount)
			{
				m_span = static_cast<GpuSpan>(static_cast<size_t>(GpuSpan::SHADOW_CASCADE_0) + static_cast<size_t>(cascade));
				m_spanActive = view.GetGpuProfiler().BeginSpan(m_span);
			}
		}
		~ShadowCascadeScope()
		{
			if (!m_counting || !DebugView::HasInstance())
			{
				return;
			}
			DebugView& view = DebugView::Get();
			if (m_spanActive)
			{
				view.GetGpuProfiler().EndSpan(m_span);
			}
			view.GetRenderStats().SetCascade(-1);
		}
		ShadowCascadeScope(const ShadowCascadeScope&) = delete;
		ShadowCascadeScope& operator=(const ShadowCascadeScope&) = delete;

	private:
		GpuSpan m_span{ GpuSpan::COUNT };
		bool m_counting{ false };
		bool m_spanActive{ false };
	};

	/**
	* @brief RAII one-shot event (design section 5.6), use through the PWG_SIM_EVENT macros. Main thread only.
	*
	* The CPU time runs from construction to End() or destruction. The GPU part is optional: either the
	* whole lifetime (second constructor) or a sub-range with BeginGpu/EndGpu. When the Debug View is
	* disabled it costs one branch and the default construction of an empty SimEvent, and records nothing.
	*/
	class SimEventScope
	{
	public:
		explicit SimEventScope(SimEventType type)
		{
			if (DebugView::IsEnabled())
			{
				DebugView::Get().GetSimulationStats().Open(m_event, type);
				m_active = true;
				m_start = DebugView::Clock::now();
			}
		}
		SimEventScope(SimEventType type, GpuEventMode mode) : SimEventScope(type)
		{
			BeginGpu(mode);
		}
		~SimEventScope() { End(); }
		SimEventScope(const SimEventScope&) = delete;
		SimEventScope& operator=(const SimEventScope&) = delete;

		bool IsActive() const { return m_active; }
		SimEvent& Event() { return m_event; }

		/** @brief Starts the GPU timer of the event. Only the first call has an effect. */
		void BeginGpu(GpuEventMode mode)
		{
			if (m_active && !m_gpuStarted && DebugView::HasInstance())
			{
				m_event.gpuEvent = DebugView::Get().GetGpuProfiler().BeginEvent(mode, &m_event.gpuTimer);
				m_gpuStarted = true;
			}
		}

		void EndGpu()
		{
			if (m_gpuStarted && !m_gpuEnded && DebugView::HasInstance())
			{
				DebugView::Get().GetGpuProfiler().EndEvent(m_event.gpuEvent);
				m_gpuEnded = true;
			}
		}

		/** @brief Drops the event: nothing is submitted, and a GPU result that arrives later is ignored. */
		void Discard()
		{
			EndGpu();
			m_active = false;
		}

		/** @brief Ends the event before the end of the scope. Later calls have no effect. */
		void End()
		{
			if (!m_active)
			{
				return;
			}
			m_active = false;
			EndGpu();
			m_event.cpuMs = std::chrono::duration<double, std::milli>(DebugView::Clock::now() - m_start).count();
			if (DebugView::HasInstance())
			{
				DebugView::Get().GetSimulationStats().Close(std::move(m_event));
			}
		}

	private:
		SimEvent m_event;
		DebugView::Clock::time_point m_start{};
		bool m_active{ false };
		bool m_gpuStarted{ false };
		bool m_gpuEnded{ false };
	};
} // namespace pwg::debug

#define PWG_DEBUG_CONCAT_INNER(a, b) a##b
#define PWG_DEBUG_CONCAT(a, b) PWG_DEBUG_CONCAT_INNER(a, b)

#if PWG_DEBUG_VIEW_INSTRUMENTATION
/** @brief Times the enclosing block on the CPU (main thread). `name` must be a string literal. */
#define PWG_CPU_SCOPE(name) ::pwg::debug::CpuScope PWG_DEBUG_CONCAT(pwgCpuScope_, __LINE__){ name }
/** @brief Times the enclosing block on the GPU as pass `pass` (a pwg::debug::GpuPass value). */
#define PWG_GPU_SCOPE(pass) ::pwg::debug::GpuScope PWG_DEBUG_CONCAT(pwgGpuScope_, __LINE__){ pass }
/** @brief Start and end TIMESTAMP of gpu_frame_ms. */
#define PWG_GPU_FRAME_START() do { if (::pwg::debug::DebugView::IsEnabled()) ::pwg::debug::DebugView::Get().GetGpuProfiler().MarkFrameStart(); } while (0)
#define PWG_GPU_FRAME_END() do { if (::pwg::debug::DebugView::IsEnabled()) ::pwg::debug::DebugView::Get().GetGpuProfiler().MarkFrameEnd(); } while (0)
/** @brief Render pass: CPU scope `name` (string literal), GPU timer of `pass` and counter slot of its draws. */
#define PWG_RENDER_PASS(pass, name) ::pwg::debug::RenderPassScope PWG_DEBUG_CONCAT(pwgRenderPass_, __LINE__){ pass, name }
/** @brief Shadow cascade `cascade` inside the shadow pass: cascade counters and the optional per-cascade span. */
#define PWG_SHADOW_CASCADE(cascade) ::pwg::debug::ShadowCascadeScope PWG_DEBUG_CONCAT(pwgShadowCascade_, __LINE__){ cascade }
/** @brief Calls a RenderStats member, e.g. PWG_RENDER_COUNT(AddDraw(n)). The arguments are evaluated only when enabled. */
#define PWG_RENDER_COUNT(call) do { if (::pwg::debug::DebugView::IsEnabled()) ::pwg::debug::DebugView::Get().GetRenderStats().call; } while (0)
/** @brief One-shot event `var` of SimEventType `type`, CPU-timed until the end of the scope or PWG_SIM_EVENT_END. */
#define PWG_SIM_EVENT(var, type) ::pwg::debug::SimEventScope var{ type }
/** @brief As PWG_SIM_EVENT, with a GPU timer (a pwg::debug::GpuEventMode) over the same range. */
#define PWG_SIM_EVENT_GPU(var, type, mode) ::pwg::debug::SimEventScope var{ type, mode }
/** @brief GPU timer over a sub-range of the event. */
#define PWG_SIM_EVENT_GPU_BEGIN(var, mode) var.BeginGpu(mode)
#define PWG_SIM_EVENT_GPU_END(var) var.EndGpu()
/** @brief Sets a SimEvent field. `value` is evaluated only when the event is recorded. */
#define PWG_SIM_EVENT_SET(var, member, value) do { if (var.IsActive()) var.Event().member = (value); } while (0)
/** @brief Runs `statement` only when the event is recorded. */
#define PWG_SIM_EVENT_DO(var, statement) do { if (var.IsActive()) { statement; } } while (0)
#define PWG_SIM_EVENT_END(var) var.End()
#define PWG_SIM_EVENT_DISCARD(var) var.Discard()
/** @brief Calls a SimulationStats member. The arguments are evaluated only when enabled. */
#define PWG_SIM_CALL(call) do { if (::pwg::debug::DebugView::IsEnabled()) ::pwg::debug::DebugView::Get().GetSimulationStats().call; } while (0)
#else
#define PWG_CPU_SCOPE(name) ((void)0)
#define PWG_GPU_SCOPE(pass) ((void)0)
#define PWG_GPU_FRAME_START() ((void)0)
#define PWG_GPU_FRAME_END() ((void)0)
#define PWG_RENDER_PASS(pass, name) ((void)0)
#define PWG_SHADOW_CASCADE(cascade) ((void)0)
#define PWG_RENDER_COUNT(call) ((void)0)
#define PWG_SIM_EVENT(var, type) ((void)0)
#define PWG_SIM_EVENT_GPU(var, type, mode) ((void)0)
#define PWG_SIM_EVENT_GPU_BEGIN(var, mode) ((void)0)
#define PWG_SIM_EVENT_GPU_END(var) ((void)0)
#define PWG_SIM_EVENT_SET(var, member, value) ((void)0)
#define PWG_SIM_EVENT_DO(var, statement) ((void)0)
#define PWG_SIM_EVENT_END(var) ((void)0)
#define PWG_SIM_EVENT_DISCARD(var) ((void)0)
#define PWG_SIM_CALL(call) ((void)0)
#endif

#endif // !SRC_DEBUG_DEBUGVIEW_H_
