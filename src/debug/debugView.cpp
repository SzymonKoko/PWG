#include <glad/glad.h>
#include "debugView.h"
#include "core/window/window.h"
#include "core/logger/logger.h"

#include <algorithm>
#include <cstdio>

pwg::debug::DebugView::DebugView(Window& window)
	: m_window(window)
{
	if (s_instance)
	{
		PWG_ERROR("DebugView: a second instance was created; the first one stays active");
		return;
	}
	s_instance = this;
	PWG_INFO("Debug View created (instrumentation {})", InstrumentationCompiled ? "compiled in" : "compiled out");
	// The window has just made the main GL context current: read GL strings and query bits once (D-09).
	m_runMetadata.CaptureContextInfo();
}

pwg::debug::DebugView::~DebugView()
{
	if (s_instance == this)
	{
		s_enabled.store(false, std::memory_order_relaxed);
		s_instance = nullptr;
	}
}

void pwg::debug::DebugView::Shutdown()
{
	s_enabled.store(false, std::memory_order_relaxed);
	if (m_recorder.IsActive())
	{
		PWG_WARN("CSV export: the application closed during a recording");
		m_recorder.Abort();
	}
	if (m_gpuInitialized)
	{
		m_gpu.Shutdown();
		m_gpuInitialized = false;
	}
}

void pwg::debug::DebugView::SetEnabled(bool enabled)
{
	if constexpr (!InstrumentationCompiled)
	{
		return;
	}
	s_enabled.store(enabled, std::memory_order_relaxed);
}

void pwg::debug::DebugView::BeginFrame()
{
	const Clock::time_point now = Clock::now();

	if (m_frameStarted)
	{
		// frame_ms of the previous frame: begin to begin, so it includes swap and limiter sleep.
		if (FrameRecord* previous = m_frameStats.Find(m_frameIndex))
		{
			previous->frameMs = std::chrono::duration<double, std::milli>(now - m_frameBegin).count();
			m_frameStats.Complete(m_frameIndex);
		}
		++m_frameIndex;
	}
	m_frameStarted = true;
	m_frameBegin = now;
	m_frameStats.BeginRecord(m_frameIndex).beginTime = now;
	m_frameStats.UpdateRolling(now);

	if constexpr (InstrumentationCompiled)
	{
		if (!m_gpuInitialized || m_requestedRingFrames != m_gpu.GetRingFrames())
		{
			m_gpu.Init(m_requestedRingFrames, &m_frameStats);
			m_requestedRingFrames = m_gpu.GetRingFrames();
			m_gpuInitialized = true;
		}
		m_gpu.BeginFrame(m_frameIndex, m_frameStats);
		// One-shot GPU events are harvested whether the Debug View is enabled or not, so the pool drains.
		m_gpuEventResults.clear();
		m_gpu.HarvestEvents(m_gpuEventResults);
		m_sim.ApplyGpuResults(m_gpuEventResults);
		m_sim.SetFrameIndex(m_frameIndex);
		m_cpu.BeginFrame(m_frameIndex);
		m_render.BeginFrame(m_frameIndex, IsEnabled());
	}

	// After the harvest above, frames up to m_frameIndex - K are final. Without instrumentation only the
	// previous frame is needed (its frame_ms was just set).
	m_recorder.OnFrameBegin(m_frameIndex, InstrumentationCompiled ? m_gpu.GetRingFrames() : 1u);
}

void pwg::debug::DebugView::EndFrame()
{
	if constexpr (InstrumentationCompiled)
	{
		m_gpu.EndFrame();
		m_cpu.EndFrame();
		m_render.EndFrame(IsEnabled(), m_frameStats.Find(m_frameIndex));
	}
}

void pwg::debug::DebugView::AddPhaseTime(FramePhase phase, Clock::duration duration)
{
	if (FrameRecord* record = m_frameStats.Find(m_frameIndex))
	{
		record->phaseMs[static_cast<size_t>(phase)] += std::chrono::duration<double, std::milli>(duration).count();
	}
}

void pwg::debug::DebugView::SetBenchmarkMode(bool enabled)
{
	if (enabled == m_benchmark)
	{
		return;
	}

	if (enabled)
	{
		m_savedFrameLimit = m_window.GetFramerateLimit();
		m_savedVSync = m_window.IsVSyncEnabled();
		m_window.SetFramerateLimit(0);
		m_window.EnableVSync(false);
		PWG_INFO("Benchmark mode on: frame limit 0, VSync off, render size {}x{}", m_benchmarkWidth, m_benchmarkHeight);
	}
	else
	{
		m_window.SetFramerateLimit(m_savedFrameLimit);
		m_window.EnableVSync(m_savedVSync);
		PWG_INFO("Benchmark mode off: frame limit {}, VSync {}", m_savedFrameLimit, m_savedVSync ? "on" : "off");
	}
	m_benchmark = enabled;
}

void pwg::debug::DebugView::SetBenchmarkRenderSize(int width, int height)
{
	m_benchmarkWidth = std::clamp(width, 16, 8192);
	m_benchmarkHeight = std::clamp(height, 16, 8192);
}

pwg::debug::Metadata pwg::debug::DebugView::CollectRunMetadata(const RunInfo& run, std::chrono::system_clock::time_point start) const
{
	Metadata metadata;
	m_runMetadata.AppendRunInfo(metadata, run, start);
	m_runMetadata.AppendBuildInfo(metadata);
	m_runMetadata.AppendMachineInfo(metadata);

	// Debug View and benchmark state (design 7.2). Values that do not apply stay empty.
	metadata.SetBool("debug_view_enabled", IsEnabled());
	metadata.SetBool("benchmark", m_benchmark);
	if (m_benchmark)
	{
		metadata.SetNumber("render_width", m_benchmarkWidth);
		metadata.SetNumber("render_height", m_benchmarkHeight);
	}
	else
	{
		metadata.SetEmpty("render_width");		// The render size follows the Main View window.
		metadata.SetEmpty("render_height");
	}
	metadata.SetNumber("frame_limit", m_window.GetFramerateLimit());
	metadata.SetNumber("swap_interval", m_window.IsVSyncEnabled() ? 1 : 0);
	if (InstrumentationCompiled)
	{
		metadata.SetNumber("gpu_ring_frames", m_gpu.GetRingFrames());
	}
	else
	{
		metadata.SetEmpty("gpu_ring_frames");
	}
	metadata.SetBool("detailed_shadow_timing", m_detailedShadowTiming);
	metadata.SetBool("wallclock_bracket", false);
	metadata.SetNumber("drain_timeout_frames", CsvRecorder::DefaultDrainTimeoutFrames);

	m_runMetadata.AppendSceneInfo(metadata);

	for (const auto& [name, value] : run.params)
	{
		metadata.Set("param." + name, value);
	}
	for (const auto& [name, value] : run.manual)
	{
		metadata.Set("manual." + name, value);
	}
	return metadata;
}

bool pwg::debug::DebugView::StartRecording(RunInfo run, const std::filesystem::path& outputRoot, uint64_t frameCount)
{
	if (m_recorder.IsActive())
	{
		return false;
	}

	const std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
	std::error_code error;
	if (run.runId.empty())
	{
		// Design 7.1: ISO local time and a counter; "m" marks a manual export from the Performance tab.
		std::string stamp = FormatIsoTime(start, false);
		std::replace(stamp.begin(), stamp.end(), ':', '-');
		do
		{
			char suffix[16];
			std::snprintf(suffix, sizeof(suffix), "_m%02u", ++m_manualExportCounter);
			run.runId = stamp + suffix;
		} while (std::filesystem::exists(outputRoot / run.runId, error));
	}

	Metadata metadata = CollectRunMetadata(run, start);
	return m_recorder.Start(run, std::move(metadata), outputRoot / run.runId, m_frameIndex, frameCount);
}
