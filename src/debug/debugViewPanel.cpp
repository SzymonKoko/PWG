#include "debugView.h"
#include "core/window/window.h"
#include "imgui.h"

#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstdio>

namespace
{
	constexpr uint32_t PlotFrames = 300;

	/** @brief Writes a millisecond value, or "-" when it is not available. */
	void ValueCell(double value)
	{
		if (std::isnan(value))
		{
			ImGui::TextDisabled("-");
		}
		else
		{
			ImGui::Text("%.3f", value);
		}
	}

	/** @brief Writes an unsigned counter. */
	void CountCell(uint64_t value)
	{
		ImGui::Text("%llu", static_cast<unsigned long long>(value));
	}

	void StatsRow(const char* name, const pwg::debug::RollingStats& stats)
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); ImGui::TextUnformatted(name);
		ImGui::TableNextColumn(); ImGui::Text("%u", stats.count);
		ImGui::TableNextColumn(); ValueCell(stats.mean);
		ImGui::TableNextColumn(); ValueCell(stats.median);
		ImGui::TableNextColumn(); ValueCell(stats.p1);
		ImGui::TableNextColumn(); ValueCell(stats.p99);
		ImGui::TableNextColumn(); ValueCell(stats.min);
		ImGui::TableNextColumn(); ValueCell(stats.max);
	}
}

void pwg::debug::DebugView::DrawPerformanceTab()
{
	// --- Switches ---
	if constexpr (InstrumentationCompiled)
	{
		bool enabled = IsEnabled();
		if (ImGui::Checkbox("Enable Debug View", &enabled))
		{
			SetEnabled(enabled);
		}
	}
	else
	{
		ImGui::TextDisabled("Instrumentation compiled out (PWG_DEBUG_VIEW_INSTRUMENTATION=OFF). Frame timing only.");
	}

	bool benchmark = m_benchmark;
	if (ImGui::Checkbox("Benchmark mode", &benchmark))
	{
		SetBenchmarkMode(benchmark);
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(m_benchmark);
	int renderSize[2] = { m_benchmarkWidth, m_benchmarkHeight };
	ImGui::SetNextItemWidth(160.0f);
	if (ImGui::InputInt2("Render size", renderSize, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		SetBenchmarkRenderSize(renderSize[0], renderSize[1]);
	}
	ImGui::EndDisabled();
	ImGui::TextDisabled("Frame limit: %u FPS%s  |  VSync: %s",
		m_window.GetFramerateLimit(),
		m_window.GetFramerateLimit() == 0 ? " (off)" : "",
		m_window.IsVSyncEnabled() ? "on" : "off");

	// --- Frame time ---
	ImGui::SeparatorText("Frame time");
	m_frameStats.CopyRecentFrameMs(m_plotScratch, PlotFrames);
	if (!m_plotScratch.empty())
	{
		char overlay[64];
		std::snprintf(overlay, sizeof(overlay), "last %zu frames, ms", m_plotScratch.size());
		ImGui::PlotLines("##frameMs", m_plotScratch.data(), static_cast<int>(m_plotScratch.size()), 0, overlay,
			0.0f, FLT_MAX, ImVec2(-1.0f, 70.0f));
	}

	const RollingStats& frameMs = m_frameStats.GetFrameMsStats();
	const RollingStats& gpuFrameMs = m_frameStats.GetGpuFrameMsStats();
	if (!std::isnan(frameMs.mean) && frameMs.mean > 0.0)
	{
		ImGui::Text("FPS (1000 / mean frame_ms): %.1f", 1000.0 / frameMs.mean);
	}
	ImGui::TextDisabled("Rolling window: last %u frames, updated twice per second. Percentiles: nearest rank.", m_frameStats.GetWindow());

	const ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;
	if (ImGui::BeginTable("RollingStats", 8, tableFlags))
	{
		ImGui::TableSetupColumn("ms");
		ImGui::TableSetupColumn("n");
		ImGui::TableSetupColumn("mean");
		ImGui::TableSetupColumn("median");
		ImGui::TableSetupColumn("p1");
		ImGui::TableSetupColumn("p99");
		ImGui::TableSetupColumn("min");
		ImGui::TableSetupColumn("max");
		ImGui::TableHeadersRow();
		StatsRow("frame_ms", frameMs);
		StatsRow("gpu_frame_ms", gpuFrameMs);
		ImGui::EndTable();
	}

	// Last complete frame: CPU phases.
	if (m_frameIndex > 0)
	{
		if (const FrameRecord* last = m_frameStats.Find(m_frameIndex - 1))
		{
			ImGui::Text("Last frame CPU (ms): update %.3f  render %.3f  swap %.3f  sleep %.3f",
				last->phaseMs[static_cast<size_t>(FramePhase::UPDATE)],
				last->phaseMs[static_cast<size_t>(FramePhase::RENDER)],
				last->phaseMs[static_cast<size_t>(FramePhase::SWAP)],
				last->phaseMs[static_cast<size_t>(FramePhase::SLEEP)]);
		}
	}

	if constexpr (InstrumentationCompiled)
	{
		DrawProfilerSection();
	}
}

void pwg::debug::DebugView::DrawProfilerSection()
{
	const ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;

	// --- GPU ---
	ImGui::SeparatorText("GPU timer queries");
	static constexpr uint32_t ringOptions[] = { 1, 2, 3, 4, 6, 8, 16 };
	const uint32_t ringFrames = m_gpu.GetRingFrames();
	char ringLabel[16];
	std::snprintf(ringLabel, sizeof(ringLabel), "%u", ringFrames);
	ImGui::SetNextItemWidth(80.0f);
	if (ImGui::BeginCombo("Ring frames (K)", ringLabel))
	{
		for (uint32_t option : ringOptions)
		{
			char optionLabel[16];
			std::snprintf(optionLabel, sizeof(optionLabel), "%u", option);
			if (ImGui::Selectable(optionLabel, option == ringFrames))
			{
				SetGpuRingFrames(option);
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	ImGui::Text("Results read: %llu  |  Dropped: %llu",
		static_cast<unsigned long long>(m_gpu.GetHarvestedCount()),
		static_cast<unsigned long long>(m_gpu.GetDroppedCount()));
	ImGui::TextDisabled("Counter bits: TIME_ELAPSED %d, TIMESTAMP %d. Results arrive K frames late and are never waited for.",
		m_gpu.GetTimeElapsedBits(), m_gpu.GetTimestampBits());
	ImGui::Checkbox("Time each shadow cascade (TIMESTAMP pairs inside the shadow pass)", &m_detailedShadowTiming);

	// CPU ms is command submission of the last counted frame; GPU ms is the latest harvested result,
	// K frames older. The two are different quantities and are never added.
	const RenderCounters* lastCounters = m_render.HasLastFrame() ? &m_render.GetLastFrame() : nullptr;
	if (ImGui::BeginTable("GpuPasses", 3, tableFlags))
	{
		ImGui::TableSetupColumn("pass");
		ImGui::TableSetupColumn("CPU ms (submit)");
		ImGui::TableSetupColumn("GPU ms (latest)");
		ImGui::TableHeadersRow();
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); ImGui::TextUnformatted("frame (timestamps)");
		ImGui::TableNextColumn(); ValueCell(NotAvailable);
		ImGui::TableNextColumn(); ValueCell(m_gpu.GetLatestFrameMs());
		for (size_t pass = 0; pass < GpuPassCount; ++pass)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::TextUnformatted(GetGpuPassName(static_cast<GpuPass>(pass)));
			ImGui::TableNextColumn(); ValueCell(lastCounters ? lastCounters->cpuPassMs[pass] : NotAvailable);
			ImGui::TableNextColumn(); ValueCell(m_gpu.GetLatestPassMs(static_cast<GpuPass>(pass)));
			if (static_cast<GpuPass>(pass) == GpuPass::SHADOW)
			{
				for (size_t span = 0; span < GpuSpanCount; ++span)
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn(); ImGui::Text("  %s", GetGpuSpanName(static_cast<GpuSpan>(span)));
					ImGui::TableNextColumn(); ValueCell(NotAvailable);
					ImGui::TableNextColumn(); ValueCell(m_gpu.GetLatestSpanMs(static_cast<GpuSpan>(span)));
				}
			}
		}
		ImGui::EndTable();
	}
	ImGui::TextDisabled("CPU ms of imgui is in the Gui::Render scope below. Cascade rows need the option above.");

	// --- CPU scopes of the last finished frame ---
	ImGui::SeparatorText("CPU scopes (last frame)");
	const std::vector<CpuScopeRecord>& scopes = m_cpu.GetLastFrameScopes();
	if (scopes.empty())
	{
		ImGui::TextDisabled(IsEnabled() ? "No scopes recorded." : "Enable the Debug View to record scopes.");
	}
	else if (ImGui::BeginTable("CpuScopes", 2, tableFlags))
	{
		ImGui::TableSetupColumn("scope");
		ImGui::TableSetupColumn("CPU ms");
		ImGui::TableHeadersRow();
		for (const CpuScopeRecord& scope : scopes)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			if (scope.depth > 0)
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 12.0f * static_cast<float>(scope.depth));
			}
			ImGui::TextUnformatted(scope.name);
			ImGui::TableNextColumn();
			ValueCell(scope.closed ? std::chrono::duration<double, std::milli>(scope.end - scope.start).count() : NotAvailable);
		}
		ImGui::EndTable();
	}
	if (m_cpu.GetOverflowCount() > 0)
	{
		ImGui::TextDisabled("Scopes not recorded (array full): %llu", static_cast<unsigned long long>(m_cpu.GetOverflowCount()));
	}

	const std::vector<CpuEvent> events = m_cpu.GetRecentEvents();
	if (!events.empty() && ImGui::TreeNode("Worker events", "Worker events (%zu)", events.size()))
	{
		for (auto it = events.rbegin(); it != events.rend(); ++it)
		{
			ImGui::Text("frame %llu  %s  %.3f ms", static_cast<unsigned long long>(it->frameIndex), it->name, it->ms);
		}
		ImGui::TreePop();
	}
}

void pwg::debug::DebugView::DrawRenderTab()
{
	if constexpr (!InstrumentationCompiled)
	{
		ImGui::TextDisabled("Instrumentation compiled out (PWG_DEBUG_VIEW_INSTRUMENTATION=OFF). No render counters.");
	}
	else
	{
		DrawRenderCounters();
	}
}

void pwg::debug::DebugView::DrawRenderCounters()
{
	if (!IsEnabled())
	{
		ImGui::TextDisabled("Enable the Debug View in the Performance tab to count draws, chunks and instances.");
	}
	if (!m_render.HasLastFrame())
	{
		ImGui::TextDisabled("No counted frame yet.");
		return;
	}

	const RenderCounters& counters = m_render.GetLastFrame();
	const ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;
	ImGui::TextDisabled("Frame %llu, the last frame counted from begin to end.", static_cast<unsigned long long>(m_render.GetLastFrameIndex()));

	// --- Draw calls and triangles per pass ---
	ImGui::SeparatorText("Draw calls and triangles");
	if (ImGui::BeginTable("RenderPasses", 3, tableFlags))
	{
		ImGui::TableSetupColumn("pass");
		ImGui::TableSetupColumn("draw calls");
		ImGui::TableSetupColumn("triangles");
		ImGui::TableHeadersRow();
		uint64_t totalDraws = 0;
		uint64_t totalTriangles = 0;
		for (size_t slot = 0; slot < CounterPassCount; ++slot)
		{
			if (slot == static_cast<size_t>(GpuPass::IMGUI))
			{
				continue;	// ImGui draws are not counted; its GPU time is in the Performance tab.
			}
			const PassCounters& pass = counters.passes[slot];
			totalDraws += pass.drawCalls;
			totalTriangles += pass.triangles;
			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::TextUnformatted(GetCounterPassName(slot));
			ImGui::TableNextColumn(); CountCell(pass.drawCalls);
			ImGui::TableNextColumn(); CountCell(pass.triangles);
		}
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); ImGui::TextUnformatted("total");
		ImGui::TableNextColumn(); CountCell(totalDraws);
		ImGui::TableNextColumn(); CountCell(totalTriangles);
		ImGui::EndTable();
	}
	ImGui::TextDisabled("Counted per GL draw command. Points add no triangles. ImGui draws are not counted.");

	// --- Shadow cascades ---
	ImGui::SeparatorText("Shadow cascades");
	if (ImGui::BeginTable("ShadowCascades", 5, tableFlags))
	{
		ImGui::TableSetupColumn("cascade");
		ImGui::TableSetupColumn("draw calls");
		ImGui::TableSetupColumn("triangles");
		ImGui::TableSetupColumn("chunks");
		ImGui::TableSetupColumn("tree instances");
		ImGui::TableHeadersRow();
		for (size_t cascade = 0; cascade < ShadowCascadeCount; ++cascade)
		{
			const CascadeCounters& values = counters.cascades[cascade];
			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::Text("%zu", cascade);
			ImGui::TableNextColumn(); CountCell(values.drawCalls);
			ImGui::TableNextColumn(); CountCell(values.triangles);
			ImGui::TableNextColumn(); CountCell(values.chunks);
			ImGui::TableNextColumn(); CountCell(values.vegetationInstances);
		}
		ImGui::EndTable();
	}

	// --- Terrain chunks ---
	ImGui::SeparatorText("Terrain chunks (main pass)");
	if (!counters.chunksCounted)
	{
		ImGui::TextDisabled("Terrain chunks were not drawn in this frame.");
	}
	else if (ImGui::BeginTable("TerrainChunks", static_cast<int>(TerrainLodCount) + 3, tableFlags))
	{
		char label[16];
		for (size_t lod = 0; lod < TerrainLodCount; ++lod)
		{
			std::snprintf(label, sizeof(label), "LOD%zu", lod);
			ImGui::TableSetupColumn(label);
		}
		ImGui::TableSetupColumn("visible");
		ImGui::TableSetupColumn("culled");
		ImGui::TableSetupColumn("total");
		ImGui::TableHeadersRow();
		ImGui::TableNextRow();
		for (size_t lod = 0; lod < TerrainLodCount; ++lod)
		{
			ImGui::TableNextColumn(); CountCell(counters.chunksVisible[lod]);
		}
		const uint32_t visible = counters.GetVisibleChunks();
		ImGui::TableNextColumn(); CountCell(visible);
		ImGui::TableNextColumn(); CountCell(counters.chunkCount > visible ? counters.chunkCount - visible : 0);
		ImGui::TableNextColumn(); CountCell(counters.chunkCount);
		ImGui::EndTable();
	}

	// --- Vegetation ---
	ImGui::SeparatorText("Vegetation instances");
	ImGui::Text("Main pass: %llu visible, %llu culled  |  Shadow pass: %llu (all cascades)",
		static_cast<unsigned long long>(counters.vegetationVisible),
		static_cast<unsigned long long>(counters.vegetationCulled),
		static_cast<unsigned long long>(counters.vegetationShadow));
	ImGui::TextDisabled("Culled covers both the distance and the frustum test (Renderer::IsSphereVisible).");
	if (ImGui::BeginTable("VegetationModels", 4, tableFlags))
	{
		ImGui::TableSetupColumn("model");
		ImGui::TableSetupColumn("visible");
		ImGui::TableSetupColumn("culled");
		ImGui::TableSetupColumn("shadow");
		ImGui::TableHeadersRow();
		for (const VegetationModelCounters& model : counters.models)
		{
			if (!model.counted)
			{
				continue;
			}
			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::TextUnformatted(model.name.c_str());
			ImGui::TableNextColumn(); CountCell(model.visible);
			ImGui::TableNextColumn(); CountCell(model.culled);
			ImGui::TableNextColumn(); CountCell(model.shadow);
		}
		ImGui::EndTable();
	}
}

namespace
{
	constexpr uint32_t RecentSimEvents = 64;

	/** @brief Opens the five-column table used for one-shot events. */
	bool BeginSimTable(const char* id)
	{
		const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;
		if (!ImGui::BeginTable(id, 5, flags))
		{
			return false;
		}
		ImGui::TableSetupColumn("event");
		ImGui::TableSetupColumn("frame");
		ImGui::TableSetupColumn("CPU ms");
		ImGui::TableSetupColumn("GPU ms");
		ImGui::TableSetupColumn("GPU timer");
		ImGui::TableHeadersRow();
		return true;
	}

	/** @brief One event row; an event that was not recorded shows "-" in every column. */
	void SimRow(const char* label, const pwg::debug::SimEvent* event)
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); ImGui::TextUnformatted(label);
		ImGui::TableNextColumn();
		if (event)
		{
			ImGui::Text("%llu", static_cast<unsigned long long>(event->frameIndex));
		}
		else
		{
			ImGui::TextDisabled("-");
		}
		ImGui::TableNextColumn(); ValueCell(event ? event->cpuMs : pwg::debug::NotAvailable);
		ImGui::TableNextColumn(); ValueCell(event ? event->gpuMs : pwg::debug::NotAvailable);
		ImGui::TableNextColumn(); ImGui::TextUnformatted(event ? pwg::debug::GetGpuTimerKindName(event->gpuTimer) : "-");
	}

	/** @brief The event if it belongs to run `run`, otherwise nullptr (an older run's value is not shown as current). */
	const pwg::debug::SimEvent* InRun(const pwg::debug::SimEvent* event, uint64_t run)
	{
		return event && event->runIndex == run ? event : nullptr;
	}
}

void pwg::debug::DebugView::DrawSimulationTab()
{
	if constexpr (!InstrumentationCompiled)
	{
		ImGui::TextDisabled("Instrumentation compiled out (PWG_DEBUG_VIEW_INSTRUMENTATION=OFF). No simulation timing.");
	}
	else
	{
		DrawSimulationTables();
	}
}

void pwg::debug::DebugView::DrawSimulationTables()
{
	const ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;

	if (!IsEnabled())
	{
		ImGui::TextDisabled("Enable the Debug View in the Performance tab to record simulation timing.");
	}
	ImGui::Text("GPU events pending: %zu  |  ELAPSED fell back to timestamps: %llu  |  refused (pool full): %llu",
		m_gpu.GetPendingEventCount(),
		static_cast<unsigned long long>(m_gpu.GetEventFallbackCount()),
		static_cast<unsigned long long>(m_gpu.GetEventRefusedCount()));
	ImGui::SameLine();
	if (ImGui::SmallButton("Clear"))
	{
		m_sim.Clear();
	}
	ImGui::TextDisabled("CPU ms: steady_clock wall time on the thread that did the work. GPU ms: timer query, read a few frames later.");
	ImGui::TextDisabled("CPU and GPU times are different quantities and are never added.");

	// --- Graph execution ---
	ImGui::SeparatorText("Graph execution (last run)");
	const uint64_t graphRun = m_sim.GetLastGraphRun();
	if (graphRun == 0)
	{
		ImGui::TextDisabled("No graph run recorded.");
	}
	else
	{
		const SimEvent* execute = InRun(m_sim.GetLatest(SimEventType::GRAPH_EXECUTE), graphRun);
		ImGui::Text("Run %llu", static_cast<unsigned long long>(graphRun));
		if (execute)
		{
			ImGui::SameLine();
			ImGui::Text("|  %d x %d  |  %s", execute->resolution, execute->resolution, execute->success ? "completed" : "returned early");
		}
		if (BeginSimTable("GraphTotals"))
		{
			SimRow("RunUniversalGraph (wall, with readback)", InRun(m_sim.GetLatest(SimEventType::GRAPH_RUN), graphRun));
			SimRow("ExecuteUniversalGraph", execute);
			SimRow("  clears + mask pass", InRun(m_sim.GetLatest(SimEventType::GRAPH_MASK_PASS), graphRun));
			SimRow("  previews", InRun(m_sim.GetLatest(SimEventType::GRAPH_PREVIEWS), graphRun));
			SimRow("height readback (sync)", InRun(m_sim.GetLatest(SimEventType::HEIGHT_READBACK_SYNC), graphRun));
			ImGui::EndTable();
		}
		ImGui::TextDisabled("ExecuteUniversalGraph GPU ms is a TIMESTAMP span over the whole call; nodes use TIME_ELAPSED.");

		const std::vector<SimEvent>& nodes = m_sim.GetLastGraphNodes();
		if (ImGui::BeginTable("GraphNodes", 7, tableFlags))
		{
			ImGui::TableSetupColumn("#");
			ImGui::TableSetupColumn("node id");
			ImGui::TableSetupColumn("type");
			ImGui::TableSetupColumn("output");
			ImGui::TableSetupColumn("CPU ms (submit)");
			ImGui::TableSetupColumn("GPU ms");
			ImGui::TableSetupColumn("GPU timer");
			ImGui::TableHeadersRow();
			for (size_t i = 0; i < nodes.size(); ++i)
			{
				const SimEvent& node = nodes[i];
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("%zu", i + 1);
				ImGui::TableNextColumn(); ImGui::Text("%d", node.nodeId);
				ImGui::TableNextColumn();
				if (node.nodeEnabled)
				{
					ImGui::TextUnformatted(node.nodeType.c_str());
				}
				else
				{
					ImGui::TextDisabled("%s (disabled)", node.nodeType.c_str());
				}
				ImGui::TableNextColumn(); ImGui::Text("%d", node.resolution);
				ImGui::TableNextColumn(); ValueCell(node.cpuMs);
				ImGui::TableNextColumn(); ValueCell(node.gpuMs);
				ImGui::TableNextColumn(); ImGui::TextUnformatted(GetGpuTimerKindName(node.gpuTimer));
			}
			ImGui::EndTable();
		}
	}

	// --- Hydraulic erosion ---
	ImGui::SeparatorText("Hydraulic erosion");
	const SimEvent* hydraulic = m_sim.GetLatest(SimEventType::HYDRAULIC_STEP);
	ImGui::Text("Steps recorded: %llu", static_cast<unsigned long long>(m_sim.GetCount(SimEventType::HYDRAULIC_STEP)));
	if (hydraulic)
	{
		ImGui::SameLine();
		ImGui::Text("|  last step %lld (solver step %lld)  |  active particles %d%s%s",
			static_cast<long long>(hydraulic->stepIndex), static_cast<long long>(hydraulic->solverStep),
			hydraulic->activeParticles, hydraulic->refreshStep ? "  |  refresh" : "", hydraulic->talusStep ? "  |  talus" : "");
	}
	if (BeginSimTable("Hydraulic"))
	{
		SimRow("step (GPU: GpuParticleErosion::Step)", hydraulic);
		SimRow("refresh (every 4th step)", m_sim.GetLatest(SimEventType::HYDRAULIC_REFRESH));
		ImGui::EndTable();
	}

	// --- Thermal erosion ---
	ImGui::SeparatorText("Thermal erosion");
	const SimEvent* thermal = m_sim.GetLatest(SimEventType::THERMAL_STEP);
	const SimEvent* check = m_sim.GetLatest(SimEventType::THERMAL_CHECK);
	const SimEvent* stable = m_sim.GetLatest(SimEventType::THERMAL_STABLE);
	ImGui::Text("Steps recorded: %llu", static_cast<unsigned long long>(m_sim.GetCount(SimEventType::THERMAL_STEP)));
	if (thermal)
	{
		ImGui::SameLine();
		ImGui::Text("|  last step %lld (solver step %lld)  |  %s%s",
			static_cast<long long>(thermal->stepIndex), static_cast<long long>(thermal->solverStep),
			thermal->stable ? "stable" : "not stable", thermal->refreshStep ? "  |  refresh" : "");
	}
	if (stable)
	{
		ImGui::Text("Became stable at solver step %lld (frame %llu)",
			static_cast<long long>(stable->solverStep), static_cast<unsigned long long>(stable->frameIndex));
	}
	if (BeginSimTable("Thermal"))
	{
		SimRow("step (GPU: GpuThermalErosion::Step)", thermal);
		SimRow("refresh (every 4th step)", m_sim.GetLatest(SimEventType::THERMAL_REFRESH));
		SimRow("stability check (sync readback)", check);
		ImGui::EndTable();
	}
	ImGui::TextDisabled("The check runs every 8th solver step and waits for the GPU. Its time is part of that step.");

	// --- Hydrology ---
	ImGui::SeparatorText("Hydrology (last run)");
	const uint64_t hydrologyRun = m_sim.GetLastHydrologyRun();
	if (hydrologyRun == 0)
	{
		ImGui::TextDisabled("No hydrology run recorded.");
	}
	else
	{
		ImGui::Text("Run %llu", static_cast<unsigned long long>(hydrologyRun));
		const SimEvent* endToEnd = InRun(m_sim.GetLatest(SimEventType::HYDROLOGY_END_TO_END), hydrologyRun);
		if (BeginSimTable("Hydrology"))
		{
			SimRow("height readback (sync, main thread)", InRun(m_sim.GetLatest(SimEventType::HYDROLOGY_READBACK), hydrologyRun));
			for (const SimEvent& stageEvent : m_sim.GetLastHydrologyStages())
			{
				if (stageEvent.runIndex != hydrologyRun)
				{
					continue;
				}
				char label[96];
				std::snprintf(label, sizeof(label), "  stage %d: %s", stageEvent.stage, stageEvent.label ? stageEvent.label : "");
				SimRow(label, &stageEvent);
			}
			SimRow("worker total", InRun(m_sim.GetLatest(SimEventType::HYDROLOGY_WORKER), hydrologyRun));
			SimRow("Start to result (main thread)", endToEnd);
			SimRow("uploads + mask pass (main thread)", InRun(m_sim.GetLatest(SimEventType::HYDROLOGY_UPLOAD), hydrologyRun));
			ImGui::EndTable();
		}
		if (endToEnd)
		{
			ImGui::TextDisabled("Start to result spans %lld frames. The result is polled once per frame, so its resolution is one frame.",
				static_cast<long long>(endToEnd->latencyFrames));
		}
		ImGui::TextDisabled("Stage times are worker wall time (steady_clock on the worker thread).");
	}

	// --- Height readback ---
	ImGui::SeparatorText("Height readback (latest)");
	const SimEvent* publish = m_sim.GetLatest(SimEventType::HEIGHT_READBACK_PUBLISH);
	if (BeginSimTable("HeightReadback"))
	{
		SimRow("UpdateHeightCache (sync)", m_sim.GetLatest(SimEventType::HEIGHT_READBACK_SYNC));
		SimRow("QueueHeightCacheReadback (PBO)", m_sim.GetLatest(SimEventType::HEIGHT_READBACK_QUEUE));
		SimRow("PollHeightCacheReadback (map + copy)", publish);
		ImGui::EndTable();
	}
	if (publish && publish->latencyFrames >= 0)
	{
		ImGui::TextDisabled("Last async readback was published %lld frames after it was queued.", static_cast<long long>(publish->latencyFrames));
	}

	// --- Recent events ---
	m_sim.CopyRecentCompleted(m_simScratch, RecentSimEvents);
	if (!m_simScratch.empty() && ImGui::TreeNode("Recent events", "Recent complete events (%zu)", m_simScratch.size()))
	{
		for (auto it = m_simScratch.rbegin(); it != m_simScratch.rend(); ++it)
		{
			ImGui::Text("frame %llu  %s  run %llu  step %lld  CPU %.3f ms  GPU %.3f ms",
				static_cast<unsigned long long>(it->frameIndex), GetSimEventName(it->type),
				static_cast<unsigned long long>(it->runIndex), static_cast<long long>(it->stepIndex), it->cpuMs, it->gpuMs);
		}
		ImGui::TreePop();
	}
}
