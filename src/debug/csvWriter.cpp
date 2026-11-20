#include "csvWriter.h"
#include "debug/debugView.h"
#include "core/logger/logger.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string_view>
#include <system_error>
#include <utility>

namespace
{
	using pwg::debug::FrameRecord;
	using pwg::debug::NotAvailable;

	enum class ValueKind : uint8_t
	{
		MS,		/**< Milliseconds, 6 decimals. */
		COUNT	/**< Integer. */
	};

	/** @brief One numeric column of frames.csv. A getter returns NaN for "not available". */
	struct FrameColumn
	{
		const char* name;
		ValueKind kind;
		double (*get)(const FrameRecord&);
		bool summarize;
		bool gpu;				/**< GPU timer value: GPU samples dropped in the window are reported with it. */
	};

	double Counter(const FrameRecord& record, uint64_t value)
	{
		return record.countersValid ? static_cast<double>(value) : NotAvailable;
	}

	constexpr size_t SlotShadow = static_cast<size_t>(pwg::debug::GpuPass::SHADOW);
	constexpr size_t SlotTerrain = static_cast<size_t>(pwg::debug::GpuPass::TERRAIN);
	constexpr size_t SlotVegetation = static_cast<size_t>(pwg::debug::GpuPass::VEGETATION);
	constexpr size_t SlotWater = static_cast<size_t>(pwg::debug::GpuPass::WATER);
	constexpr size_t SlotPost = static_cast<size_t>(pwg::debug::GpuPass::POST_PROCESS);
	constexpr size_t SlotImgui = static_cast<size_t>(pwg::debug::GpuPass::IMGUI);
	constexpr size_t SlotOther = pwg::debug::OtherCounterPass;

	uint64_t DrawsMain(const FrameRecord& r) { return uint64_t{ r.drawCalls[SlotTerrain] } + r.drawCalls[SlotVegetation] + r.drawCalls[SlotWater]; }
	uint64_t TrisMain(const FrameRecord& r) { return r.triangles[SlotTerrain] + r.triangles[SlotVegetation] + r.triangles[SlotWater]; }
	uint64_t DrawsTotal(const FrameRecord& r)
	{
		uint64_t sum = 0;
		for (uint32_t value : r.drawCalls) { sum += value; }
		return sum;
	}
	uint64_t TrisTotal(const FrameRecord& r)
	{
		uint64_t sum = 0;
		for (uint64_t value : r.triangles) { sum += value; }
		return sum;
	}
	uint64_t ChunksVisible(const FrameRecord& r)
	{
		uint64_t sum = 0;
		for (uint32_t value : r.chunksVisible) { sum += value; }
		return sum;
	}
	double Phase(const FrameRecord& r, pwg::debug::FramePhase phase) { return r.phaseMs[static_cast<size_t>(phase)]; }

	/**
	* frames.csv columns after the identifier columns (design section 7.3). The order is part of the format;
	* new columns are only appended (format_version stays), see docs/debug_view_export.md.
	*/
	const FrameColumn FrameColumns[] = {
		{ "frame_ms", ValueKind::MS, +[](const FrameRecord& r) { return r.frameMs; }, true, false },
		{ "cpu_update_ms", ValueKind::MS, +[](const FrameRecord& r) { return Phase(r, pwg::debug::FramePhase::UPDATE); }, true, false },
		{ "cpu_render_ms", ValueKind::MS, +[](const FrameRecord& r) { return Phase(r, pwg::debug::FramePhase::RENDER); }, true, false },
		{ "cpu_swap_ms", ValueKind::MS, +[](const FrameRecord& r) { return Phase(r, pwg::debug::FramePhase::SWAP); }, true, false },
		{ "cpu_sleep_ms", ValueKind::MS, +[](const FrameRecord& r) { return Phase(r, pwg::debug::FramePhase::SLEEP); }, true, false },
		{ "gpu_frame_ms", ValueKind::MS, +[](const FrameRecord& r) { return r.gpuFrameMs; }, true, true },
		{ "gpu_shadow_ms", ValueKind::MS, +[](const FrameRecord& r) { return r.gpuPassMs[SlotShadow]; }, true, true },
		{ "gpu_terrain_ms", ValueKind::MS, +[](const FrameRecord& r) { return r.gpuPassMs[SlotTerrain]; }, true, true },
		{ "gpu_vegetation_ms", ValueKind::MS, +[](const FrameRecord& r) { return r.gpuPassMs[SlotVegetation]; }, true, true },
		{ "gpu_water_ms", ValueKind::MS, +[](const FrameRecord& r) { return r.gpuPassMs[SlotWater]; }, true, true },
		{ "gpu_post_ms", ValueKind::MS, +[](const FrameRecord& r) { return r.gpuPassMs[SlotPost]; }, true, true },
		{ "gpu_imgui_ms", ValueKind::MS, +[](const FrameRecord& r) { return r.gpuPassMs[SlotImgui]; }, true, true },
		{ "gpu_shadow_c0_ms", ValueKind::MS, +[](const FrameRecord& r) { return r.gpuSpanMs[0]; }, true, true },
		{ "gpu_shadow_c1_ms", ValueKind::MS, +[](const FrameRecord& r) { return r.gpuSpanMs[1]; }, true, true },
		{ "gpu_shadow_c2_ms", ValueKind::MS, +[](const FrameRecord& r) { return r.gpuSpanMs[2]; }, true, true },
		{ "draw_calls_total", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, DrawsTotal(r)); }, true, false },
		{ "draw_calls_shadow", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.drawCalls[SlotShadow]); }, true, false },
		{ "draw_calls_main", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, DrawsMain(r)); }, true, false },
		{ "draw_calls_terrain", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.drawCalls[SlotTerrain]); }, true, false },
		{ "draw_calls_vegetation", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.drawCalls[SlotVegetation]); }, true, false },
		{ "draw_calls_water", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.drawCalls[SlotWater]); }, true, false },
		{ "draw_calls_post", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.drawCalls[SlotPost]); }, true, false },
		{ "draw_calls_other", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.drawCalls[SlotOther]); }, true, false },
		{ "tris_total", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, TrisTotal(r)); }, true, false },
		{ "tris_shadow", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.triangles[SlotShadow]); }, true, false },
		{ "tris_main", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, TrisMain(r)); }, true, false },
		{ "tris_terrain", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.triangles[SlotTerrain]); }, true, false },
		{ "tris_vegetation", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.triangles[SlotVegetation]); }, true, false },
		{ "tris_water", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.triangles[SlotWater]); }, true, false },
		{ "tris_post", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.triangles[SlotPost]); }, true, false },
		{ "tris_other", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.triangles[SlotOther]); }, true, false },
		{ "chunks_visible", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, ChunksVisible(r)); }, true, false },
		{ "chunks_lod0", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.chunksVisible[0]); }, true, false },
		{ "chunks_lod1", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.chunksVisible[1]); }, true, false },
		{ "chunks_lod2", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.chunksVisible[2]); }, true, false },
		{ "chunks_lod3", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.chunksVisible[3]); }, true, false },
		{ "chunks_lod4", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.chunksVisible[4]); }, true, false },
		{ "chunks_culled", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.chunksCulled); }, true, false },
		{ "veg_visible", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.vegetationVisible); }, true, false },
		{ "veg_culled", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.vegetationCulled); }, true, false },
		{ "veg_shadow", ValueKind::COUNT, +[](const FrameRecord& r) { return Counter(r, r.vegetationShadow); }, true, false },
		{ "gpu_dropped", ValueKind::COUNT, +[](const FrameRecord& r)
			{ return pwg::debug::InstrumentationCompiled ? static_cast<double>(r.gpuDropped) : NotAvailable; }, false, false },
	};

	static_assert(pwg::debug::GpuSpanCount == 3, "frames.csv has one column per shadow cascade span");
	static_assert(pwg::debug::TerrainLodCount == 5, "frames.csv has one column per LOD level");

	/** @brief Path as UTF-8 for log messages (path::string() uses the ANSI code page and can throw). */
	std::string PathText(const std::filesystem::path& path)
	{
		const std::u8string text = path.u8string();
		return std::string(text.begin(), text.end());
	}

	/** @brief Value as written to the CSV; empty for NaN. */
	std::string FormatValue(double value, ValueKind kind)
	{
		if (std::isnan(value))
		{
			return {};
		}
		char text[48];
		if (kind == ValueKind::MS)
		{
			std::snprintf(text, sizeof(text), "%.6f", value);
		}
		else
		{
			std::snprintf(text, sizeof(text), "%.0f", value);
		}
		return text;
	}

	/** @brief The value a reader gets back from the CSV text, so app summaries match recomputed ones. */
	double Quantize(double value, ValueKind kind)
	{
		const std::string text = FormatValue(value, kind);
		return text.empty() ? NotAvailable : std::strtod(text.c_str(), nullptr);
	}

	ValueKind KindOfUnit(const char* unit)
	{
		return std::string_view(unit) == "ms" ? ValueKind::MS : ValueKind::COUNT;
	}

	/**
	* @brief Text field: quoted if it holds a comma, quote or line break. '#' is replaced by a space,
	* because readers that use '#' as the comment character (pandas comment="#") would cut the row there.
	*/
	std::string Text(const std::string& value)
	{
		std::string clean = value;
		bool quote = false;
		for (char& c : clean)
		{
			if (c == '#')
			{
				c = ' ';
			}
			else if (c == ',' || c == '"' || c == '\n' || c == '\r')
			{
				quote = true;
			}
		}
		if (!quote)
		{
			return clean;
		}
		std::string quoted = "\"";
		for (char c : clean)
		{
			if (c == '"')
			{
				quoted += '"';
			}
			quoted += c;
		}
		quoted += '"';
		return quoted;
	}

	/** @brief Value inside a `key=value;...` detail string: the separators are replaced by spaces. */
	std::string DetailValue(const std::string& value)
	{
		std::string clean = value;
		for (char& c : clean)
		{
			if (c == ';' || c == '=' || c == '\n' || c == '\r')
			{
				c = ' ';
			}
		}
		return clean;
	}

	void AddDetail(std::string& detail, const char* key, const std::string& value)
	{
		if (!detail.empty())
		{
			detail += ';';
		}
		detail += key;
		detail += '=';
		detail += DetailValue(value);
	}

	void AddDetail(std::string& detail, const char* key, int64_t value)
	{
		AddDetail(detail, key, std::to_string(value));
	}

	const char* TimerSource(pwg::debug::GpuTimerKind kind)
	{
		switch (kind)
		{
		case pwg::debug::GpuTimerKind::TIME_ELAPSED: return "time_elapsed";
		case pwg::debug::GpuTimerKind::TIMESTAMP_PAIR: return "timestamp_pair";
		default: return "";
		}
	}

	/** @brief Summary statistics of one metric (design 7.3). Percentiles: nearest rank, integer arithmetic. */
	struct Summary
	{
		uint64_t n{ 0 };
		double mean{ NotAvailable };
		double median{ NotAvailable };
		double p1{ NotAvailable };
		double p5{ NotAvailable };
		double p95{ NotAvailable };
		double p99{ NotAvailable };
		double min{ NotAvailable };
		double max{ NotAvailable };
		double sd{ NotAvailable };
	};

	Summary Summarize(std::vector<double>& values)
	{
		Summary s;
		s.n = values.size();
		if (values.empty())
		{
			return s;
		}
		std::sort(values.begin(), values.end());
		double sum = 0.0;
		for (double v : values) { sum += v; }
		s.mean = sum / static_cast<double>(s.n);
		// rank = ceil(p * n / 100) in integer arithmetic, clamped to [1, n].
		auto percentile = [&](uint64_t p)
		{
			uint64_t rank = (p * s.n + 99) / 100;
			rank = std::clamp<uint64_t>(rank, 1, s.n);
			return values[rank - 1];
		};
		s.median = percentile(50);
		s.p1 = percentile(1);
		s.p5 = percentile(5);
		s.p95 = percentile(95);
		s.p99 = percentile(99);
		s.min = values.front();
		s.max = values.back();
		if (s.n >= 2)
		{
			double squares = 0.0;
			for (double v : values) { squares += (v - s.mean) * (v - s.mean); }
			s.sd = std::sqrt(squares / static_cast<double>(s.n - 1));
		}
		return s;
	}

	std::string Stat(double value)
	{
		return FormatValue(value, ValueKind::MS);
	}

	/** @brief First columns of every data row: run_id, variant, repetition. */
	std::string RunPrefix(const pwg::debug::RunInfo& run)
	{
		return Text(run.runId) + ',' + Text(run.variant) + ',' + std::to_string(run.repetition);
	}
}

void pwg::debug::AppendSimEventRows(const SimEvent& event, int64_t order, std::vector<EventRow>& out)
{
	EventRow base;
	base.engineFrame = event.frameIndex;
	base.event = GetSimEventName(event.type);
	base.sequence = event.sequence;
	base.group = event.runIndex;

	if (event.type == SimEventType::GRAPH_NODE)
	{
		base.index = order;
	}
	else if (event.stage >= 0)
	{
		base.index = event.stage;
	}
	else
	{
		base.index = event.stepIndex;
	}

	std::string& detail = base.detail;
	if (event.nodeId >= 0)
	{
		AddDetail(detail, "node_id", event.nodeId);
		AddDetail(detail, "node_type", event.nodeType);
		AddDetail(detail, "node_enabled", event.nodeEnabled ? 1 : 0);
	}
	if (event.resolution > 0)
	{
		AddDetail(detail, "resolution", event.resolution);
	}
	if (event.solverStep >= 0)
	{
		AddDetail(detail, "solver_step", event.solverStep);
	}
	if (event.label)
	{
		AddDetail(detail, "stage_name", std::string(event.label));
	}
	switch (event.type)
	{
	case SimEventType::GRAPH_RUN:
	case SimEventType::GRAPH_EXECUTE:
		AddDetail(detail, "success", event.success ? 1 : 0);
		break;
	case SimEventType::HYDRAULIC_STEP:
		AddDetail(detail, "refresh", event.refreshStep ? 1 : 0);
		AddDetail(detail, "talus", event.talusStep ? 1 : 0);
		break;
	case SimEventType::THERMAL_STEP:
		AddDetail(detail, "refresh", event.refreshStep ? 1 : 0);
		AddDetail(detail, "stable", event.stable ? 1 : 0);
		break;
	case SimEventType::THERMAL_CHECK:
		AddDetail(detail, "stable", event.stable ? 1 : 0);
		break;
	default:
		break;
	}

	const size_t first = out.size();
	auto addRow = [&](const char* metric, double value, const char* unit, const char* source)
	{
		EventRow row = base;
		row.metric = metric;
		row.value = value;
		row.unit = unit;
		row.source = source;
		out.push_back(std::move(row));
	};

	if (!std::isnan(event.cpuMs))
	{
		addRow("cpu_ms", event.cpuMs, "ms", "steady_clock");
	}
	if (event.gpuTimer != GpuTimerKind::NONE)
	{
		// Written even without a result (empty value), so a GPU time that never arrived stays visible.
		addRow("gpu_ms", event.gpuMs, "ms", TimerSource(event.gpuTimer));
	}
	if (event.activeParticles >= 0)
	{
		addRow("active_particles", static_cast<double>(event.activeParticles), "1", "");
	}
	if (event.latencyFrames >= 0)
	{
		addRow("latency_frames", static_cast<double>(event.latencyFrames), "frame", "");
	}
	if (event.type == SimEventType::THERMAL_STABLE)
	{
		addRow("solver_step", static_cast<double>(event.solverStep), "1", "");
	}
	if (out.size() == first)
	{
		addRow("occurred", 1.0, "1", "");	// Every event has at least one row.
	}
}

// --- CsvRecorder ---

pwg::debug::CsvRecorder::CsvRecorder(const FrameStats& frameStats, SimulationStats& simulationStats)
	: m_frameStats(frameStats), m_sim(simulationStats)
{
}

pwg::debug::CsvRecorder::~CsvRecorder()
{
	if (m_state != State::IDLE)
	{
		m_sim.SetCapture(nullptr);
	}
}

bool pwg::debug::CsvRecorder::Start(const RunInfo& run, Metadata metadata, std::filesystem::path folder, uint64_t currentFrame, uint64_t frameCount)
{
	if (m_state != State::IDLE)
	{
		return false;
	}
	std::error_code error;
	for (const char* name : { "frames.csv", "events.csv", "summary.csv", "run.yaml" })
	{
		// The folder may exist: the runner exports masks and screenshots into it before the window starts.
		if (std::filesystem::exists(folder / name, error))
		{
			PWG_ERROR("CSV export: {} exists already in {}", name, PathText(folder));
			return false;
		}
	}

	Reset();
	m_run = run;
	m_metadata = std::move(metadata);
	m_folder = std::move(folder);
	m_firstFrame = currentFrame + 1;
	m_frameCount = std::min<uint64_t>(frameCount, MaxFrames);
	m_lastFrame = m_frameCount > 0 ? m_firstFrame + m_frameCount - 1 : UINT64_MAX;
	m_nextFrame = m_firstFrame;
	// Reserve up front, so the window does not reallocate for the usual sizes.
	m_frames.reserve(static_cast<size_t>(m_frameCount > 0 ? m_frameCount : 8192));
	m_events.reserve(4096);
	m_sim.SetCapture(&m_events);
	m_state = State::RECORDING;
	PWG_INFO("CSV export: recording from frame {} ({}), output {}", m_firstFrame,
		m_frameCount > 0 ? std::to_string(m_frameCount) + " frames" : std::string("until stopped"), PathText(m_folder));
	return true;
}

void pwg::debug::CsvRecorder::Stop(uint64_t currentFrame)
{
	if (m_state != State::RECORDING)
	{
		return;
	}
	const uint64_t last = std::max(currentFrame, m_firstFrame);
	if (last < m_lastFrame)
	{
		m_lastFrame = last;
		m_stopReason = StopReason::USER;
	}
}

bool pwg::debug::CsvRecorder::AddEventRow(EventRow row)
{
	if (m_state != State::RECORDING || row.engineFrame < m_firstFrame || row.engineFrame > m_lastFrame)
	{
		return false;
	}
	m_extraRows.push_back(std::move(row));
	return true;
}

void pwg::debug::CsvRecorder::Abort()
{
	if (m_state == State::IDLE)
	{
		return;
	}
	PWG_INFO("CSV export: recording aborted, nothing written");
	m_sim.SetCapture(nullptr);
	Reset();
}

void pwg::debug::CsvRecorder::Reset()
{
	m_state = State::IDLE;
	m_stopReason = StopReason::NONE;
	m_run = RunInfo{};
	m_metadata = Metadata{};
	m_folder.clear();
	m_firstFrame = 0;
	m_lastFrame = UINT64_MAX;
	m_frameCount = 0;
	m_nextFrame = 0;
	m_missingFrames = 0;
	m_drainStartFrame = 0;
	m_eventLimitReached = false;
	// Release the memory of a long window.
	std::vector<FrameRecord>().swap(m_frames);
	std::vector<SimEvent>().swap(m_events);
	m_extraRows.clear();
}

void pwg::debug::CsvRecorder::Advance(uint64_t frameIndex, uint32_t lagFrames)
{
	// Frames up to frameIndex - lag have their frame_ms, CPU phases, counters and GPU results (or drops).
	const uint64_t lag = std::max<uint32_t>(lagFrames, 1);
	if (frameIndex >= lag)
	{
		const uint64_t ready = frameIndex - lag;
		while (m_nextFrame <= ready && m_nextFrame <= m_lastFrame)
		{
			if (m_frames.size() >= MaxFrames)
			{
				m_lastFrame = m_nextFrame - 1;
				m_stopReason = StopReason::FRAME_LIMIT;
				break;
			}
			if (const FrameRecord* record = m_frameStats.Find(m_nextFrame))
			{
				m_frames.push_back(*record);
			}
			else
			{
				++m_missingFrames;
			}
			++m_nextFrame;
		}
	}

	if (!m_eventLimitReached && m_events.size() >= MaxEvents)
	{
		m_eventLimitReached = true;
		m_sim.SetCapture(nullptr);
	}

	if (m_state == State::RECORDING && m_lastFrame != UINT64_MAX && frameIndex > m_lastFrame)
	{
		if (m_stopReason == StopReason::NONE)
		{
			m_stopReason = StopReason::FRAME_COUNT;
		}
		m_state = State::DRAINING;
		m_drainStartFrame = frameIndex;
	}

	if (m_state == State::DRAINING)
	{
		const bool framesDone = m_nextFrame > m_lastFrame;
		const bool eventsDone = m_sim.CountPending(m_firstFrame, m_lastFrame) == 0;
		const bool timedOut = frameIndex - m_drainStartFrame >= DefaultDrainTimeoutFrames;
		if (framesDone && (eventsDone || timedOut))
		{
			Finish(frameIndex);
		}
	}
}

void pwg::debug::CsvRecorder::Finish(uint64_t frameIndex)
{
	ExportResult result = Write(frameIndex);
	m_sim.SetCapture(nullptr);
	if (result.written)
	{
		PWG_INFO("CSV export: {} frames, {} event rows, {} summary rows written to {} in {:.1f} ms",
			result.frames, result.eventRows, result.summaryRows, PathText(result.folder), result.writeMs);
	}
	else
	{
		PWG_ERROR("CSV export failed: {}", result.error);
	}
	for (const std::string& warning : result.warnings)
	{
		PWG_WARN("CSV export: {}", warning);
	}
	m_lastResult = std::move(result);
	m_hasResult = true;
	Reset();
}

pwg::debug::ExportResult pwg::debug::CsvRecorder::Write(uint64_t frameIndex)
{
	const Clock::time_point writeStart = Clock::now();
	ExportResult result;
	result.folder = m_folder;

	// --- Events of the window, in the order they were opened ---
	std::vector<SimEvent> events;
	events.reserve(m_events.size());
	for (const SimEvent& event : m_events)
	{
		if (event.frameIndex >= m_firstFrame && event.frameIndex <= m_lastFrame)
		{
			events.push_back(event);
		}
	}
	const size_t completeEvents = events.size();
	m_sim.CopyPending(events, m_firstFrame, m_lastFrame);
	const uint64_t pendingEvents = events.size() - completeEvents;
	std::sort(events.begin(), events.end(), [](const SimEvent& a, const SimEvent& b) { return a.sequence < b.sequence; });

	std::vector<EventRow> eventRows;
	eventRows.reserve(events.size() * 2);
	std::map<uint64_t, int64_t> nodeOrder;
	for (const SimEvent& event : events)
	{
		int64_t order = -1;
		if (event.type == SimEventType::GRAPH_NODE)
		{
			order = nodeOrder[event.runIndex]++;
		}
		AppendSimEventRows(event, order, eventRows);
	}
	if (!m_extraRows.empty())
	{
		// Rows of other producers (the runner) take their place by sequence number; rows of one event stay together.
		for (const EventRow& row : m_extraRows)
		{
			if (row.engineFrame >= m_firstFrame && row.engineFrame <= m_lastFrame)
			{
				eventRows.push_back(row);
			}
		}
		std::stable_sort(eventRows.begin(), eventRows.end(), [](const EventRow& a, const EventRow& b) { return a.sequence < b.sequence; });
	}

	// --- Warnings ---
	uint64_t gpuDropped = 0;
	for (const FrameRecord& record : m_frames)
	{
		gpuDropped += record.gpuDropped;
	}
	if (m_missingFrames > 0)
	{
		result.warnings.push_back(std::to_string(m_missingFrames) + " frames had left the frame ring before they were copied");
	}
	if (pendingEvents > 0)
	{
		result.warnings.push_back(std::to_string(pendingEvents) + " events still waited for their GPU result; gpu_ms is empty for them");
	}
	if (m_eventLimitReached)
	{
		result.warnings.push_back("event limit reached; later events of the window are missing");
	}
	if (m_stopReason == StopReason::FRAME_LIMIT)
	{
		result.warnings.push_back("frame limit reached; the window was cut");
	}
	if (gpuDropped > 0)
	{
		result.warnings.push_back(std::to_string(gpuDropped) + " GPU samples were dropped; repeat with more ring frames (K)");
	}

	result.gpuDropped = InstrumentationCompiled ? gpuDropped : 0;
	for (const FrameRecord& record : m_frames)
	{
		const double sleepMs = record.phaseMs[static_cast<size_t>(FramePhase::SLEEP)];
		if (!std::isnan(sleepMs) && sleepMs > result.maxSleepMs)
		{
			result.maxSleepMs = sleepMs;
		}
	}

	std::error_code error;
	std::filesystem::create_directories(m_folder, error);
	if (error)
	{
		result.error = "cannot create " + PathText(m_folder) + ": " + error.message();
		return result;
	}

	const std::string prefix = RunPrefix(m_run);
	const double firstBegin = m_frames.empty() ? 0.0 :
		std::chrono::duration<double, std::milli>(m_frames.front().beginTime.time_since_epoch()).count();

	// --- frames.csv ---
	{
		std::ofstream out(m_folder / "frames.csv", std::ios::binary);
		if (!out.is_open())
		{
			result.error = "cannot write frames.csv";
			return result;
		}
		WriteMetadataComments(out, m_metadata, "frames.csv");
		out << "run_id,variant,repetition,frame,engine_frame,path_frame,t_ms";
		for (const FrameColumn& column : FrameColumns)
		{
			out << ',' << column.name;
		}
		out << '\n';
		std::string line;
		for (size_t i = 0; i < m_frames.size(); ++i)
		{
			const FrameRecord& record = m_frames[i];
			const double beginMs = std::chrono::duration<double, std::milli>(record.beginTime.time_since_epoch()).count();
			line = prefix;
			line += ',' + std::to_string(record.frameIndex - m_firstFrame);
			line += ',' + std::to_string(record.frameIndex);
			line += ',';
			if (record.pathFrame >= 0)
			{
				line += std::to_string(record.pathFrame);	// camera path frame applied in this frame (D-10); empty without a path
			}
			line += ',' + FormatValue(beginMs - firstBegin, ValueKind::MS);
			for (const FrameColumn& column : FrameColumns)
			{
				line += ',';
				line += FormatValue(column.get(record), column.kind);
			}
			line += '\n';
			out << line;
		}
		if (!out.good())
		{
			result.error = "write error in frames.csv";
			return result;
		}
	}

	// --- events.csv ---
	{
		std::ofstream out(m_folder / "events.csv", std::ios::binary);
		if (!out.is_open())
		{
			result.error = "cannot write events.csv";
			return result;
		}
		WriteMetadataComments(out, m_metadata, "events.csv");
		out << "run_id,variant,repetition,frame,engine_frame,event,seq,group,index,detail,metric,value,unit,source\n";
		std::string line;
		for (const EventRow& row : eventRows)
		{
			line = prefix;
			line += ',' + std::to_string(row.engineFrame - m_firstFrame);
			line += ',' + std::to_string(row.engineFrame);
			line += ',' + Text(row.event);
			line += ',' + std::to_string(row.sequence);
			line += ',' + (row.group > 0 ? std::to_string(row.group) : std::string());
			line += ',' + (row.index >= 0 ? std::to_string(row.index) : std::string());
			line += ',' + Text(row.detail);
			line += ',' + Text(row.metric);
			line += ',' + FormatValue(row.value, KindOfUnit(row.unit));
			line += ',' + Text(row.unit);
			line += ',' + Text(row.source);
			line += '\n';
			out << line;
		}
		if (!out.good())
		{
			result.error = "write error in events.csv";
			return result;
		}
	}

	// --- summary.csv: computed from the values as written, so a recomputation from the CSV matches ---
	{
		std::ofstream out(m_folder / "summary.csv", std::ios::binary);
		if (!out.is_open())
		{
			result.error = "cannot write summary.csv";
			return result;
		}
		WriteMetadataComments(out, m_metadata, "summary.csv");
		out << "run_id,variant,repetition,source,metric,n,missing,mean,median,p1,p5,p95,p99,min,max,sd,dropped";
		std::string params;
		for (const auto& [name, value] : m_run.params)
		{
			out << ',' << Text("param." + name);
			params += ',' + Text(value);
		}
		out << '\n';

		auto writeRow = [&](const char* source, const std::string& metric, std::vector<double>& values, uint64_t rows, uint64_t dropped)
		{
			const Summary s = Summarize(values);
			if (s.n == 0)
			{
				return;		// Metric not measured in this window (e.g. GPU columns with the Debug View off).
			}
			out << prefix << ',' << source << ',' << Text(metric) << ',' << s.n << ',' << (rows - s.n)
				<< ',' << Stat(s.mean) << ',' << Stat(s.median) << ',' << Stat(s.p1) << ',' << Stat(s.p5)
				<< ',' << Stat(s.p95) << ',' << Stat(s.p99) << ',' << Stat(s.min) << ',' << Stat(s.max)
				<< ',' << Stat(s.sd) << ',' << dropped << params << '\n';
			++result.summaryRows;
		};

		std::vector<double> values;
		values.reserve(m_frames.size());
		for (const FrameColumn& column : FrameColumns)
		{
			if (!column.summarize)
			{
				continue;
			}
			values.clear();
			for (const FrameRecord& record : m_frames)
			{
				const double value = Quantize(column.get(record), column.kind);
				if (!std::isnan(value))
				{
					values.push_back(value);
				}
			}
			writeRow("frames", column.name, values, m_frames.size(), column.gpu ? gpuDropped : 0);
		}

		// Events: one row per (event, metric) with unit ms, in order of first appearance.
		std::vector<std::pair<std::string, std::string>> keys;
		for (const EventRow& row : eventRows)
		{
			if (KindOfUnit(row.unit) != ValueKind::MS)
			{
				continue;
			}
			const std::pair<std::string, std::string> key{ row.event, row.metric };
			if (std::find(keys.begin(), keys.end(), key) == keys.end())
			{
				keys.push_back(key);
			}
		}
		for (const auto& key : keys)
		{
			values.clear();
			uint64_t rows = 0;
			for (const EventRow& row : eventRows)
			{
				if (row.event == key.first && row.metric == key.second)
				{
					++rows;
					const double value = Quantize(row.value, ValueKind::MS);
					if (!std::isnan(value))
					{
						values.push_back(value);
					}
				}
			}
			// For gpu_ms, a missing value is a GPU result that did not arrive before the export.
			const uint64_t dropped = key.second == "gpu_ms" ? rows - values.size() : 0;
			writeRow("events", key.first + "." + key.second, values, rows, dropped);
		}
		if (!out.good())
		{
			result.error = "write error in summary.csv";
			return result;
		}
	}

	result.frames = m_frames.size();
	result.eventRows = eventRows.size();
	result.writeMs = std::chrono::duration<double, std::milli>(Clock::now() - writeStart).count();

	// --- run.yaml ---
	Metadata status;
	status.Set("status", "complete");
	const char* stopReason = "user";
	switch (m_stopReason)
	{
	case StopReason::FRAME_COUNT: stopReason = "frame_count"; break;
	case StopReason::FRAME_LIMIT: stopReason = "frame_limit"; break;
	default: break;
	}
	status.Set("stop_reason", stopReason);
	status.SetNumber("window_first_engine_frame", m_firstFrame);
	status.SetNumber("window_last_engine_frame", m_lastFrame);
	status.SetNumber("frames", static_cast<uint64_t>(m_frames.size()));
	status.SetNumber("frames_missing", m_missingFrames);
	status.SetNumber("events", static_cast<uint64_t>(events.size()));
	status.SetNumber("event_rows", static_cast<uint64_t>(eventRows.size()));
	status.SetNumber("events_pending_at_export", pendingEvents);
	status.SetBool("event_limit_reached", m_eventLimitReached);
	if (InstrumentationCompiled)
	{
		status.SetNumber("gpu_dropped_total", gpuDropped);
	}
	else
	{
		status.SetEmpty("gpu_dropped_total");
	}
	status.SetNumber("summary_rows", result.summaryRows);
	status.SetNumber("drain_frames", frameIndex - m_drainStartFrame);
	status.SetNumber("csv_write_ms", result.writeMs);
	std::string warnings;
	for (const std::string& warning : result.warnings)
	{
		warnings += (warnings.empty() ? "" : "; ") + warning;
	}
	status.Set("warnings", warnings);

	if (!WriteRunYaml(m_folder / "run.yaml", m_metadata, status, { "frames.csv", "events.csv", "summary.csv" }))
	{
		result.error = "cannot write run.yaml";
		return result;
	}
	result.written = true;
	return result;
}
