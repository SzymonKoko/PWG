#include "cpuProfiler.h"

pwg::debug::CpuProfiler::CpuProfiler()
	: m_mainThread(std::this_thread::get_id())
{
	m_lastFrame.reserve(MaxScopesPerFrame);
	m_pendingEvents.reserve(RecentEventCount);
}

void pwg::debug::CpuProfiler::BeginFrame(uint64_t frameIndex)
{
	m_frameIndex = frameIndex;
	m_count = 0;
	m_open = -1;
}

void pwg::debug::CpuProfiler::EndFrame()
{
	m_lastFrame.assign(m_current.begin(), m_current.begin() + m_count);

	std::lock_guard<std::mutex> lock(m_eventMutex);
	for (CpuEvent& event : m_pendingEvents)
	{
		event.frameIndex = m_frameIndex;
		m_recentEvents[m_recentHead] = event;
		m_recentHead = (m_recentHead + 1) % RecentEventCount;
		if (m_recentCount < RecentEventCount)
		{
			++m_recentCount;
		}
	}
	m_pendingEvents.clear();
}

int32_t pwg::debug::CpuProfiler::BeginScope(const char* name)
{
	if (std::this_thread::get_id() != m_mainThread)
	{
		return -1;
	}
	if (m_count >= MaxScopesPerFrame)
	{
		++m_overflow;
		return -1;
	}

	const int32_t index = static_cast<int32_t>(m_count++);
	CpuScopeRecord& record = m_current[index];
	record.name = name;
	record.parent = static_cast<int16_t>(m_open);
	record.depth = m_open >= 0 ? static_cast<uint16_t>(m_current[m_open].depth + 1) : 0;
	record.closed = false;
	m_open = index;
	record.start = Clock::now();
	return index;
}

void pwg::debug::CpuProfiler::EndScope(int32_t index, uint64_t frameIndex)
{
	const Clock::time_point end = Clock::now();
	// A scope that outlived its frame is dropped: its slot already belongs to the new frame.
	if (index < 0 || frameIndex != m_frameIndex || static_cast<uint32_t>(index) >= m_count)
	{
		return;
	}

	CpuScopeRecord& record = m_current[index];
	record.end = end;
	record.closed = true;
	m_open = record.parent;
}

void pwg::debug::CpuProfiler::SubmitEvent(const char* name, Clock::time_point start, Clock::time_point end)
{
	CpuEvent event;
	event.name = name;
	event.ms = std::chrono::duration<double, std::milli>(end - start).count();

	std::lock_guard<std::mutex> lock(m_eventMutex);
	m_pendingEvents.push_back(event);
}

std::vector<pwg::debug::CpuEvent> pwg::debug::CpuProfiler::GetRecentEvents() const
{
	std::vector<CpuEvent> events;
	events.reserve(m_recentCount);
	const uint32_t first = (m_recentHead + RecentEventCount - m_recentCount) % RecentEventCount;
	for (uint32_t i = 0; i < m_recentCount; ++i)
	{
		events.push_back(m_recentEvents[(first + i) % RecentEventCount]);
	}
	return events;
}
