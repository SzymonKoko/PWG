#include <glad/glad.h>
#include "gpuProfiler.h"
#include "core/logger/logger.h"

#include <algorithm>
#include <cmath>

void pwg::debug::GpuProfiler::Init(uint32_t ringFrames, FrameStats* stats)
{
	if (!m_slots.empty())
	{
		// Last non-blocking harvest of the old ring before its query objects are deleted.
		for (Slot& slot : m_slots)
		{
			HarvestSlot(slot, stats);
		}
		DeleteQueries();
	}

	m_slots.assign(std::clamp<uint32_t>(ringFrames, 1, MaxRingFrames), Slot{});
	for (Slot& slot : m_slots)
	{
		// glCreateQueries gives query objects whose result is "available" with value 0 (spec 4.2).
		// The issued flags below make sure such a never-used result is not read.
		glCreateQueries(GL_TIME_ELAPSED, static_cast<GLsizei>(GpuPassCount), slot.passQueries.data());
		glCreateQueries(GL_TIMESTAMP, 1, &slot.frameStartQuery);
		glCreateQueries(GL_TIMESTAMP, 1, &slot.frameEndQuery);
		glCreateQueries(GL_TIMESTAMP, static_cast<GLsizei>(slot.spanQueries.size()), slot.spanQueries.data());
		slot.frameIndex = UINT64_MAX;
	}

	GLint bits = 0;
	glGetQueryiv(GL_TIME_ELAPSED, GL_QUERY_COUNTER_BITS, &bits);
	m_timeElapsedBits = bits;
	bits = 0;
	glGetQueryiv(GL_TIMESTAMP, GL_QUERY_COUNTER_BITS, &bits);
	m_timestampBits = bits;

	m_current = nullptr;
	m_activePass = GpuPass::COUNT;
	m_latestPassMs.fill(NotAvailable);
	m_latestFrameMs = NotAvailable;
	m_latestSpanMs.fill(NotAvailable);

	PWG_INFO("GPU profiler: ring of {} frames, counter bits TIME_ELAPSED={} TIMESTAMP={}",
		m_slots.size(), m_timeElapsedBits, m_timestampBits);
}

void pwg::debug::GpuProfiler::Shutdown()
{
	if (m_activePass != GpuPass::COUNT)
	{
		glEndQuery(GL_TIME_ELAPSED);
		m_activePass = GpuPass::COUNT;
	}
	DeleteQueries();
	m_slots.clear();
	m_current = nullptr;
}

void pwg::debug::GpuProfiler::DeleteQueries()
{
	for (Slot& slot : m_slots)
	{
		glDeleteQueries(static_cast<GLsizei>(GpuPassCount), slot.passQueries.data());
		glDeleteQueries(1, &slot.frameStartQuery);
		glDeleteQueries(1, &slot.frameEndQuery);
		glDeleteQueries(static_cast<GLsizei>(slot.spanQueries.size()), slot.spanQueries.data());
	}
}

void pwg::debug::GpuProfiler::BeginFrame(uint64_t frameIndex, FrameStats& stats)
{
	if (m_slots.empty())
	{
		m_current = nullptr;
		return;
	}

	Slot& slot = m_slots[frameIndex % m_slots.size()];
	HarvestSlot(slot, &stats);
	slot.frameIndex = frameIndex;
	m_current = &slot;
}

void pwg::debug::GpuProfiler::EndFrame()
{
	if (m_activePass != GpuPass::COUNT)
	{
		if (!m_reportedMisuse)
		{
			PWG_ERROR("GPU profiler: pass '{}' still active at frame end", GetGpuPassName(m_activePass));
			m_reportedMisuse = true;
		}
		glEndQuery(GL_TIME_ELAPSED);
		m_activePass = GpuPass::COUNT;
	}
}

bool pwg::debug::GpuProfiler::BeginPass(GpuPass pass)
{
	const size_t index = static_cast<size_t>(pass);
	if (!m_current || index >= GpuPassCount)
	{
		return false;
	}

	// Only one TIME_ELAPSED query may be active (spec 4.2), and a pass is timed once per frame,
	// because a second BeginQuery on the same object would replace the first result.
	if (m_activePass != GpuPass::COUNT || m_current->passIssued[index])
	{
		if (!m_reportedMisuse)
		{
			PWG_ERROR("GPU profiler: pass '{}' not timed (nested or repeated TIME_ELAPSED scope)", GetGpuPassName(pass));
			m_reportedMisuse = true;
		}
		return false;
	}

	glBeginQuery(GL_TIME_ELAPSED, m_current->passQueries[index]);
	m_current->passIssued[index] = true;
	m_activePass = pass;
	return true;
}

void pwg::debug::GpuProfiler::EndPass(GpuPass pass)
{
	if (m_activePass != pass)
	{
		return;
	}
	glEndQuery(GL_TIME_ELAPSED);
	m_activePass = GpuPass::COUNT;
}

void pwg::debug::GpuProfiler::MarkFrameStart()
{
	if (!m_current || m_current->frameStartIssued)
	{
		return;
	}
	// A TIMESTAMP may be issued while a TIME_ELAPSED query is active (spec 4.3).
	glQueryCounter(m_current->frameStartQuery, GL_TIMESTAMP);
	m_current->frameStartIssued = true;
}

void pwg::debug::GpuProfiler::MarkFrameEnd()
{
	if (!m_current || !m_current->frameStartIssued || m_current->frameEndIssued)
	{
		return;
	}
	glQueryCounter(m_current->frameEndQuery, GL_TIMESTAMP);
	m_current->frameEndIssued = true;
}

bool pwg::debug::GpuProfiler::BeginSpan(GpuSpan span)
{
	const size_t index = static_cast<size_t>(span);
	if (!m_current || index >= GpuSpanCount)
	{
		return false;
	}
	// A second begin in the same frame would overwrite the first TIMESTAMP of the span.
	if (m_current->spanState[index] != 0)
	{
		if (!m_reportedMisuse)
		{
			PWG_ERROR("GPU profiler: span '{}' not timed (repeated in one frame)", GetGpuSpanName(span));
			m_reportedMisuse = true;
		}
		return false;
	}
	// A TIMESTAMP may be issued while a TIME_ELAPSED query is active (spec 4.3), so a span can lie inside a pass.
	glQueryCounter(m_current->spanQueries[index * 2], GL_TIMESTAMP);
	m_current->spanState[index] = 1;
	return true;
}

void pwg::debug::GpuProfiler::EndSpan(GpuSpan span)
{
	const size_t index = static_cast<size_t>(span);
	if (!m_current || index >= GpuSpanCount || m_current->spanState[index] != 1)
	{
		return;
	}
	glQueryCounter(m_current->spanQueries[index * 2 + 1], GL_TIMESTAMP);
	m_current->spanState[index] = 2;
}

bool pwg::debug::GpuProfiler::ReadIfAvailable(unsigned int query, uint64_t& result)
{
	GLint available = GL_FALSE;
	glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &available);
	if (available != GL_TRUE)
	{
		return false;
	}
	// The result is available, so this read returns immediately (spec 4.2.1).
	GLuint64 value = 0;
	glGetQueryObjectui64v(query, GL_QUERY_RESULT, &value);
	result = value;
	return true;
}

double pwg::debug::GpuProfiler::ReadTimestampSpan(unsigned int startQuery, unsigned int endQuery)
{
	uint64_t startNs = 0;
	uint64_t endNs = 0;
	// Check the end first: if it is available, the earlier start is too (spec 4.2.1), but both are
	// checked so that no read can wait.
	if (ReadIfAvailable(endQuery, endNs) && ReadIfAvailable(startQuery, startNs))
	{
		return static_cast<double>(endNs - startNs) * 1.0e-6;
	}
	return NotAvailable;
}

void pwg::debug::GpuProfiler::CountDropped(FrameRecord* record)
{
	++m_dropped;
	if (record)
	{
		++record->gpuDropped;
	}
}

void pwg::debug::GpuProfiler::HarvestSlot(Slot& slot, FrameStats* stats)
{
	FrameRecord* record = stats ? stats->Find(slot.frameIndex) : nullptr;

	for (size_t pass = 0; pass < GpuPassCount; ++pass)
	{
		if (!slot.passIssued[pass])
		{
			continue;
		}
		slot.passIssued[pass] = false;

		uint64_t ns = 0;
		if (ReadIfAvailable(slot.passQueries[pass], ns))
		{
			const double ms = static_cast<double>(ns) * 1.0e-6;
			m_latestPassMs[pass] = ms;
			++m_harvested;
			if (record)
			{
				record->gpuPassMs[pass] = ms;
			}
		}
		else
		{
			CountDropped(record);
		}
	}

	if (slot.frameStartIssued && slot.frameEndIssued)
	{
		const double ms = ReadTimestampSpan(slot.frameStartQuery, slot.frameEndQuery);
		if (!std::isnan(ms))
		{
			m_latestFrameMs = ms;
			++m_harvested;
			if (record)
			{
				record->gpuFrameMs = ms;
			}
		}
		else
		{
			CountDropped(record);
		}
	}
	slot.frameStartIssued = false;
	slot.frameEndIssued = false;

	for (size_t span = 0; span < GpuSpanCount; ++span)
	{
		const uint8_t state = slot.spanState[span];
		slot.spanState[span] = 0;
		if (state == 0)
		{
			continue;
		}
		// A span that was begun but never ended has no result and counts as dropped.
		const double ms = state == 2 ? ReadTimestampSpan(slot.spanQueries[span * 2], slot.spanQueries[span * 2 + 1]) : NotAvailable;
		if (!std::isnan(ms))
		{
			m_latestSpanMs[span] = ms;
			++m_harvested;
			if (record)
			{
				record->gpuSpanMs[span] = ms;
			}
		}
		else
		{
			CountDropped(record);
		}
	}
}
