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
