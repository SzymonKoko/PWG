#include "frameStats.h"

#include <algorithm>
#include <cmath>

namespace
{
	/**
	* @brief Nearest-rank percentile: the value at rank ceil(p / 100 * n) of the sorted sample.
	* Uses nth_element, so the sample is reordered but never fully sorted.
	*/
	double NearestRank(std::vector<double>& samples, double percentile)
	{
		const size_t n = samples.size();
		size_t rank = static_cast<size_t>(std::ceil(percentile / 100.0 * static_cast<double>(n)));
		rank = std::clamp<size_t>(rank, 1, n);
		auto nth = samples.begin() + static_cast<std::ptrdiff_t>(rank - 1);
		std::nth_element(samples.begin(), nth, samples.end());
		return *nth;
	}
}

const char* pwg::debug::GetGpuPassName(GpuPass pass)
{
	switch (pass)
	{
	case GpuPass::SHADOW: return "shadow";
	case GpuPass::TERRAIN: return "terrain";
	case GpuPass::VEGETATION: return "vegetation";
	case GpuPass::WATER: return "water";
	case GpuPass::POST_PROCESS: return "post";
	case GpuPass::IMGUI: return "imgui";
	default: return "unknown";
	}
}

const char* pwg::debug::GetGpuSpanName(GpuSpan span)
{
	switch (span)
	{
	case GpuSpan::SHADOW_CASCADE_0: return "shadow cascade 0";
	case GpuSpan::SHADOW_CASCADE_1: return "shadow cascade 1";
	case GpuSpan::SHADOW_CASCADE_2: return "shadow cascade 2";
	default: return "unknown";
	}
}

const char* pwg::debug::GetCounterPassName(size_t slot)
{
	return slot < GpuPassCount ? GetGpuPassName(static_cast<GpuPass>(slot)) : "other";
}

pwg::debug::FrameStats::FrameStats(uint32_t capacity, uint32_t window)
	: m_records(std::max<uint32_t>(capacity, 1)),
	  m_window(std::clamp<uint32_t>(window, 1, std::max<uint32_t>(capacity, 1)))
{
	m_scratch.reserve(m_window);
	// Mark every slot as unused so Find() never matches a frame that was not recorded.
	for (FrameRecord& record : m_records)
	{
		record.frameIndex = UINT64_MAX;
	}
}

pwg::debug::FrameRecord& pwg::debug::FrameStats::BeginRecord(uint64_t frameIndex)
{
	FrameRecord& record = m_records[frameIndex % m_records.size()];
	record = FrameRecord{};
	record.frameIndex = frameIndex;
	record.gpuPassMs.fill(NotAvailable);
	record.gpuSpanMs.fill(NotAvailable);
	return record;
}

pwg::debug::FrameRecord* pwg::debug::FrameStats::Find(uint64_t frameIndex)
{
	FrameRecord& record = m_records[frameIndex % m_records.size()];
	return record.frameIndex == frameIndex ? &record : nullptr;
}

const pwg::debug::FrameRecord* pwg::debug::FrameStats::Find(uint64_t frameIndex) const
{
	const FrameRecord& record = m_records[frameIndex % m_records.size()];
	return record.frameIndex == frameIndex ? &record : nullptr;
}

void pwg::debug::FrameStats::UpdateRolling(Clock::time_point now, bool force)
{
	if (!force && now - m_lastRollingUpdate < std::chrono::milliseconds(500))
	{
		return;
	}
	m_lastRollingUpdate = now;

	if (!m_hasComplete)
	{
		return;
	}

	const uint64_t count = std::min<uint64_t>(m_window, m_lastComplete + 1);
	const uint64_t first = m_lastComplete + 1 - count;

	m_scratch.clear();
	for (uint64_t frame = first; frame <= m_lastComplete; ++frame)
	{
		const FrameRecord* record = Find(frame);
		if (record && !std::isnan(record->frameMs))
		{
			m_scratch.push_back(record->frameMs);
		}
	}
	m_frameMsStats = Compute(m_scratch);

	m_scratch.clear();
	for (uint64_t frame = first; frame <= m_lastComplete; ++frame)
	{
		const FrameRecord* record = Find(frame);
		if (record && !std::isnan(record->gpuFrameMs))
		{
			m_scratch.push_back(record->gpuFrameMs);
		}
	}
	m_gpuFrameMsStats = Compute(m_scratch);
}

void pwg::debug::FrameStats::CopyRecentFrameMs(std::vector<float>& out, uint32_t count) const
{
	out.clear();
	if (!m_hasComplete)
	{
		return;
	}

	const uint64_t n = std::min<uint64_t>({ count, m_records.size(), m_lastComplete + 1 });
	for (uint64_t frame = m_lastComplete + 1 - n; frame <= m_lastComplete; ++frame)
	{
		const FrameRecord* record = Find(frame);
		out.push_back(record && !std::isnan(record->frameMs) ? static_cast<float>(record->frameMs) : 0.0f);
	}
}

pwg::debug::RollingStats pwg::debug::FrameStats::Compute(std::vector<double>& samples)
{
	RollingStats stats;
	if (samples.empty())
	{
		return stats;
	}

	double sum = 0.0;
	double minValue = samples.front();
	double maxValue = samples.front();
	for (double value : samples)
	{
		sum += value;
		minValue = std::min(minValue, value);
		maxValue = std::max(maxValue, value);
	}

	stats.count = static_cast<uint32_t>(samples.size());
	stats.mean = sum / static_cast<double>(samples.size());
	stats.min = minValue;
	stats.max = maxValue;
	stats.median = NearestRank(samples, 50.0);
	stats.p1 = NearestRank(samples, 1.0);
	stats.p99 = NearestRank(samples, 99.0);
	return stats;
}
