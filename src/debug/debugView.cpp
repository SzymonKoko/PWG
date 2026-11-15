#include <glad/glad.h>
#include "debugView.h"
#include "core/window/window.h"
#include "core/logger/logger.h"

#include <algorithm>

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
	m_frameStats.BeginRecord(m_frameIndex);
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
		m_cpu.BeginFrame(m_frameIndex);
		m_render.BeginFrame(m_frameIndex, IsEnabled());
	}
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
