#include "simulationStats.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
	bool OpensGraphRun(pwg::debug::SimEventType type)
	{
		return type == pwg::debug::SimEventType::GRAPH_RUN || type == pwg::debug::SimEventType::GRAPH_EXECUTE;
	}

	bool IsHydrologyEvent(pwg::debug::SimEventType type)
	{
		using pwg::debug::SimEventType;
		return type == SimEventType::HYDROLOGY_READBACK || type == SimEventType::HYDROLOGY_STAGE ||
			type == SimEventType::HYDROLOGY_WORKER || type == SimEventType::HYDROLOGY_END_TO_END ||
			type == SimEventType::HYDROLOGY_UPLOAD;
	}
}

const char* pwg::debug::GetSimEventName(SimEventType type)
{
	switch (type)
	{
	case SimEventType::GRAPH_RUN: return "graph_run";
	case SimEventType::GRAPH_EXECUTE: return "graph_execute";
	case SimEventType::GRAPH_NODE: return "graph_node";
	case SimEventType::GRAPH_MASK_PASS: return "graph_mask_pass";
	case SimEventType::GRAPH_PREVIEWS: return "graph_previews";
	case SimEventType::HEIGHT_READBACK_SYNC: return "height_readback_sync";
	case SimEventType::HEIGHT_READBACK_QUEUE: return "height_readback_queue";
	case SimEventType::HEIGHT_READBACK_PUBLISH: return "height_readback_publish";
	case SimEventType::HYDRAULIC_STEP: return "hydraulic_step";
	case SimEventType::HYDRAULIC_REFRESH: return "hydraulic_refresh";
	case SimEventType::THERMAL_STEP: return "thermal_step";
	case SimEventType::THERMAL_REFRESH: return "thermal_refresh";
	case SimEventType::THERMAL_CHECK: return "thermal_check";
	case SimEventType::THERMAL_STABLE: return "thermal_stable";
	case SimEventType::HYDROLOGY_READBACK: return "hydrology_readback";
	case SimEventType::HYDROLOGY_STAGE: return "hydrology_stage";
	case SimEventType::HYDROLOGY_WORKER: return "hydrology_worker";
	case SimEventType::HYDROLOGY_END_TO_END: return "hydrology_end_to_end";
	case SimEventType::HYDROLOGY_UPLOAD: return "hydrology_upload";
	default: return "?";
	}
}

pwg::debug::SimulationStats::SimulationStats()
{
	m_recent.reserve(RecentEventCount);
}

void pwg::debug::SimulationStats::Open(SimEvent& event, SimEventType type)
{
	event.type = type;
	event.sequence = ++m_sequence;
	event.frameIndex = m_frameIndex;
	if (OpensGraphRun(type))
	{
		// The outermost graph event starts a new run; events opened inside it belong to that run.
		if (m_graphDepth == 0)
		{
			++m_graphRun;
			m_lastGraphNodes.clear();
		}
		++m_graphDepth;
		event.runIndex = m_graphRun;
	}
	else if (IsHydrologyEvent(type))
	{
		event.runIndex = m_hydrologyRun;
	}
	else if (m_graphDepth > 0)
	{
		event.runIndex = m_graphRun;
	}
}

void pwg::debug::SimulationStats::Close(SimEvent&& event)
{
	const size_t type = static_cast<size_t>(event.type);
	if (type >= SimEventTypeCount)
	{
		return;
	}
	if (OpensGraphRun(event.type) && m_graphDepth > 0)
	{
		--m_graphDepth;
	}

	++m_counts[type];
	UpdateViews(event);
	if (event.gpuEvent != 0)
	{
		m_pending.push_back(std::move(event));
	}
	else
	{
		Complete(std::move(event));
	}
}

void pwg::debug::SimulationStats::UpdateViews(const SimEvent& event)
{
	const size_t type = static_cast<size_t>(event.type);
	m_latest[type] = event;
	m_hasLatest[type] = true;
	if (event.type == SimEventType::GRAPH_NODE && event.runIndex == m_graphRun)
	{
		m_lastGraphNodes.push_back(event);
	}
}

void pwg::debug::SimulationStats::ApplyGpuResults(const std::vector<GpuEventResult>& results)
{
	for (const GpuEventResult& result : results)
	{
		auto it = std::find_if(m_pending.begin(), m_pending.end(), [&](const SimEvent& event)
		{
			return event.gpuEvent == result.id;
		});
		if (it == m_pending.end())
		{
			continue;	// Cleared in the meantime.
		}

		const size_t type = static_cast<size_t>(it->type);
		if (m_hasLatest[type] && m_latest[type].gpuEvent == result.id)
		{
			m_latest[type].gpuMs = result.ms;
		}
		if (it->type == SimEventType::GRAPH_NODE)
		{
			for (SimEvent& node : m_lastGraphNodes)
			{
				if (node.gpuEvent == result.id)
				{
					node.gpuMs = result.ms;
					break;
				}
			}
		}

		SimEvent event = std::move(*it);
		m_pending.erase(it);
		event.gpuMs = result.ms;
		Complete(std::move(event));
	}
}

void pwg::debug::SimulationStats::Complete(SimEvent&& event)
{
	if (m_capture)
	{
		m_capture->push_back(event);
	}
	if (m_recent.size() < RecentEventCount)
	{
		m_recent.push_back(std::move(event));
	}
	else
	{
		m_recent[m_recentHead] = std::move(event);
	}
	m_recentHead = (m_recentHead + 1) % RecentEventCount;
	++m_completedTotal;
}

void pwg::debug::SimulationStats::SubmitThermalStable(int64_t solverStep)
{
	SimEvent event;
	Open(event, SimEventType::THERMAL_STABLE);
	event.solverStep = solverStep;
	event.stable = true;
	Close(std::move(event));
}

void pwg::debug::SimulationStats::BeginHydrologyRun()
{
	++m_hydrologyRun;
	m_hydrologyStartValid = false;
}

void pwg::debug::SimulationStats::MarkHydrologyStarted()
{
	m_hydrologyStart = Clock::now();
	m_hydrologyStartFrame = m_frameIndex;
	m_hydrologyStartValid = true;
}

void pwg::debug::SimulationStats::SubmitHydrologyResult(const double* stageMs, size_t stageCount, const char* const* stageNames,
	double resampleMs, double workerMs)
{
	const Clock::time_point now = Clock::now();
	m_lastHydrologyStages.clear();

	auto submitStage = [&](int32_t stage, const char* name, double ms)
	{
		if (std::isnan(ms))
		{
			return;		// The stage did not run (flat terrain ends after stage 1).
		}
		SimEvent event;
		Open(event, SimEventType::HYDROLOGY_STAGE);
		event.stage = stage;
		event.label = name;
		event.cpuMs = ms;
		m_lastHydrologyStages.push_back(event);
		Close(std::move(event));
	};

	// Stage 0 is the resampling at the start of stage 1, reported on its own as section 5.6 asks.
	submitStage(0, "Resampling (part of stage 1)", resampleMs);
	for (size_t i = 0; i < stageCount; ++i)
	{
		submitStage(static_cast<int32_t>(i + 1), stageNames[i], stageMs[i]);
	}

	SimEvent worker;
	Open(worker, SimEventType::HYDROLOGY_WORKER);
	worker.cpuMs = workerMs;
	Close(std::move(worker));

	if (m_hydrologyStartValid)
	{
		SimEvent endToEnd;
		Open(endToEnd, SimEventType::HYDROLOGY_END_TO_END);
		endToEnd.frameIndex = m_hydrologyStartFrame;
		endToEnd.cpuMs = std::chrono::duration<double, std::milli>(now - m_hydrologyStart).count();
		endToEnd.latencyFrames = static_cast<int64_t>(m_frameIndex - m_hydrologyStartFrame);
		Close(std::move(endToEnd));
	}
	m_hydrologyStartValid = false;
}

void pwg::debug::SimulationStats::Clear()
{
	m_pending.clear();
	m_latest = {};
	m_hasLatest = {};
	m_counts = {};
	m_lastGraphNodes.clear();
	m_lastHydrologyStages.clear();
	m_recent.clear();
	m_recentHead = 0;
}

size_t pwg::debug::SimulationStats::CountPending(uint64_t firstFrame, uint64_t lastFrame) const
{
	size_t count = 0;
	for (const SimEvent& event : m_pending)
	{
		if (event.frameIndex >= firstFrame && event.frameIndex <= lastFrame)
		{
			++count;
		}
	}
	return count;
}

void pwg::debug::SimulationStats::CopyPending(std::vector<SimEvent>& out, uint64_t firstFrame, uint64_t lastFrame) const
{
	for (const SimEvent& event : m_pending)
	{
		if (event.frameIndex >= firstFrame && event.frameIndex <= lastFrame)
		{
			out.push_back(event);
		}
	}
}

const pwg::debug::SimEvent* pwg::debug::SimulationStats::GetLatest(SimEventType type) const
{
	const size_t index = static_cast<size_t>(type);
	return index < SimEventTypeCount && m_hasLatest[index] ? &m_latest[index] : nullptr;
}

void pwg::debug::SimulationStats::CopyRecentCompleted(std::vector<SimEvent>& out, uint32_t count) const
{
	out.clear();
	const size_t size = m_recent.size();
	const size_t n = std::min<size_t>(count, size);
	// Before the ring is full, the oldest element is at 0; afterwards it is at the head.
	const size_t oldest = size < RecentEventCount ? 0 : m_recentHead;
	for (size_t i = size - n; i < size; ++i)
	{
		out.push_back(m_recent[(oldest + i) % size]);
	}
}
