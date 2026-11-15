#ifndef SRC_DEBUG_DEBUGVIEW_H_
#define SRC_DEBUG_DEBUGVIEW_H_

#include <atomic>
#include <chrono>
#include <cstdint>
#include <vector>
#include "debug/cpuProfiler.h"
#include "debug/frameStats.h"
#include "debug/gpuProfiler.h"
#include "debug/renderStats.h"

// Set by CMake (option PWG_DEBUG_VIEW_INSTRUMENTATION, default ON).
#ifndef PWG_DEBUG_VIEW_INSTRUMENTATION
#define PWG_DEBUG_VIEW_INSTRUMENTATION 1
#endif

namespace pwg
{
	class Window;
}

namespace pwg::debug
{
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

		/** @brief Detailed option: one TIMESTAMP span per shadow cascade inside the shadow pass timer. Off by default. */
		void SetDetailedShadowTiming(bool enabled) { m_detailedShadowTiming = enabled; }
		bool IsDetailedShadowTiming() const { return m_detailedShadowTiming; }

		uint64_t GetFrameIndex() const { return m_frameIndex; }
		CpuProfiler& GetCpuProfiler() { return m_cpu; }
		GpuProfiler& GetGpuProfiler() { return m_gpu; }
		RenderStats& GetRenderStats() { return m_render; }
		const FrameStats& GetFrameStats() const { return m_frameStats; }

		/** @brief "Performance" tab of the Debug window (debugViewPanel.cpp). */
		void DrawPerformanceTab();

		/** @brief "Render" tab of the Debug window: render counters (debugViewPanel.cpp). */
		void DrawRenderTab();

	private:
		/** @brief GPU query and CPU scope part of the Performance tab (instrumented builds only). */
		void DrawProfilerSection();

		/** @brief Counter tables of the Render tab (instrumented builds only). */
		void DrawRenderCounters();

		static inline DebugView* s_instance{ nullptr };
		static inline std::atomic<bool> s_enabled{ false };

		Window& m_window;
		CpuProfiler m_cpu;
		GpuProfiler m_gpu;
		RenderStats m_render;
		FrameStats m_frameStats;

		uint64_t m_frameIndex{ 0 };
		bool m_frameStarted{ false };
		bool m_gpuInitialized{ false };
		uint32_t m_requestedRingFrames{ GpuProfiler::DefaultRingFrames };
		bool m_detailedShadowTiming{ false };
		Clock::time_point m_frameBegin{};

		bool m_benchmark{ false };
		int m_benchmarkWidth{ 1920 };
		int m_benchmarkHeight{ 1080 };
		unsigned int m_savedFrameLimit{ 0 };
		bool m_savedVSync{ false };

		std::vector<float> m_plotScratch;		/**< Used by DrawPerformanceTab only. */
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
#else
#define PWG_CPU_SCOPE(name) ((void)0)
#define PWG_GPU_SCOPE(pass) ((void)0)
#define PWG_GPU_FRAME_START() ((void)0)
#define PWG_GPU_FRAME_END() ((void)0)
#define PWG_RENDER_PASS(pass, name) ((void)0)
#define PWG_SHADOW_CASCADE(cascade) ((void)0)
#define PWG_RENDER_COUNT(call) ((void)0)
#endif

#endif // !SRC_DEBUG_DEBUGVIEW_H_
