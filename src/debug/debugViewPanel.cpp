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

	if (ImGui::BeginTable("GpuPasses", 2, tableFlags))
	{
		ImGui::TableSetupColumn("pass");
		ImGui::TableSetupColumn("GPU ms (latest)");
		ImGui::TableHeadersRow();
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); ImGui::TextUnformatted("frame (timestamps)");
		ImGui::TableNextColumn(); ValueCell(m_gpu.GetLatestFrameMs());
		for (size_t pass = 0; pass < GpuPassCount; ++pass)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::TextUnformatted(GetGpuPassName(static_cast<GpuPass>(pass)));
			ImGui::TableNextColumn(); ValueCell(m_gpu.GetLatestPassMs(static_cast<GpuPass>(pass)));
		}
		ImGui::EndTable();
	}

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
