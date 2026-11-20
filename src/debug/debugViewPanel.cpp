#include <glad/glad.h>
#include "debugView.h"
#include "experimentRunner.h"
#include "core/window/window.h"
#include "imgui.h"
#include "terrain/chunk/chunkManager.h"

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>

namespace
{
	constexpr uint32_t PlotFrames = 300;

	/** @brief Path as UTF-8 for ImGui (path::string() uses the ANSI code page and can throw). */
	std::string Utf8(const std::filesystem::path& path)
	{
		const std::u8string text = path.u8string();
		return std::string(text.begin(), text.end());
	}

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
	// Settings are locked during a CSV recording: the metadata header describes the whole window.
	const bool recording = m_recorder.IsActive();
	ImGui::BeginDisabled(recording);
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

	DrawExportSection();

	if constexpr (InstrumentationCompiled)
	{
		DrawProfilerSection();
	}
}

void pwg::debug::DebugView::DrawExportSection()
{
	ImGui::SeparatorText("CSV export");
	const bool active = m_recorder.IsActive();

	ImGui::BeginDisabled(active);
	ImGui::SetNextItemWidth(260.0f);
	ImGui::InputText("Output folder", m_exportRoot, sizeof(m_exportRoot));
	ImGui::SetNextItemWidth(120.0f);
	if (ImGui::InputInt("Frames (0 = until Stop)", &m_exportFrames, 100, 1000))
	{
		m_exportFrames = std::max(m_exportFrames, 0);
	}
	ImGui::EndDisabled();

	std::error_code error;
	const std::filesystem::path root = std::filesystem::absolute(std::filesystem::path(m_exportRoot), error);
	ImGui::TextDisabled("Writes <folder>/<run id>/frames.csv, events.csv, summary.csv and run.yaml. Folder: %s",
		error ? m_exportRoot : Utf8(root).c_str());

	if (!active)
	{
		if (ImGui::Button("Start recording"))
		{
			StartRecording(RunInfo{}, std::filesystem::path(m_exportRoot), static_cast<uint64_t>(m_exportFrames));
		}
	}
	else if (m_recorder.GetState() == CsvRecorder::State::RECORDING)
	{
		if (ImGui::Button("Stop and export"))
		{
			StopRecording();
		}
		ImGui::SameLine();
		if (ImGui::Button("Abort"))
		{
			AbortRecording();
		}
		ImGui::SameLine();
		if (m_recorder.GetFrameCount() > 0)
		{
			ImGui::Text("Recording: %llu / %llu frames, %llu events",
				static_cast<unsigned long long>(m_recorder.GetRecordedFrames()),
				static_cast<unsigned long long>(m_recorder.GetFrameCount()),
				static_cast<unsigned long long>(m_recorder.GetCapturedEvents()));
		}
		else
		{
			ImGui::Text("Recording: %llu frames, %llu events",
				static_cast<unsigned long long>(m_recorder.GetRecordedFrames()),
				static_cast<unsigned long long>(m_recorder.GetCapturedEvents()));
		}
	}
	else
	{
		ImGui::Text("Waiting for the last GPU results of the window, then writing...");
	}
	ImGui::TextDisabled("Settings above are locked while recording. Files are written after the window, never inside it.");

	if (m_recorder.HasResult())
	{
		const ExportResult& result = m_recorder.GetLastResult();
		if (result.written)
		{
			ImGui::Text("Last export: %llu frames, %llu event rows, %llu summary rows (%.1f ms to write)",
				static_cast<unsigned long long>(result.frames), static_cast<unsigned long long>(result.eventRows),
				static_cast<unsigned long long>(result.summaryRows), result.writeMs);
			const std::string folder = Utf8(result.folder);
			ImGui::TextWrapped("%s", folder.c_str());
			ImGui::SameLine();
			if (ImGui::SmallButton("Copy path"))
			{
				ImGui::SetClipboardText(folder.c_str());
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Last export failed: %s", result.error.c_str());
		}
		for (const std::string& warning : result.warnings)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Warning: %s", warning.c_str());
		}
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
	ImGui::BeginDisabled(m_recorder.IsActive());
	const bool ringCombo = ImGui::BeginCombo("Ring frames (K)", ringLabel);
	ImGui::EndDisabled();
	if (ringCombo)
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
	ImGui::BeginDisabled(m_recorder.IsActive());
	ImGui::Checkbox("Time each shadow cascade (TIMESTAMP pairs inside the shadow pass)", &m_detailedShadowTiming);
	ImGui::EndDisabled();

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

void pwg::debug::DebugView::DrawChunkLodSection(ChunkManager* chunks)
{
	if (chunks == nullptr)
	{
		ImGui::TextDisabled("No terrain, no chunk and LOD controls.");
		return;
	}

	// --- LOD ---
	ImGui::SeparatorText("LOD");
	bool lodEnabled = chunks->IsLodEnabled();
	if (ImGui::Checkbox("LOD enabled", &lodEnabled))
	{
		chunks->SetLodEnabled(lodEnabled);
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(off: every chunk uses LOD0)");
	if (!lodEnabled)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f),
			"All chunks at full detail, also in the shadow passes. A large grid may stall the GPU (note in LODDistances.h).");
	}

	float scale = chunks->GetLodScale();
	if (ImGui::SliderFloat("Distance scale", &scale, 0.1f, 4.0f, "%.2f", ImGuiSliderFlags_Logarithmic))
	{
		chunks->SetLodScale(scale);	// rejected values keep the old scale
	}
	const float appliedScale = chunks->GetLodScale();

	LODManager::Thresholds distances = chunks->GetLodDistances();
	bool distancesChanged = false;
	if (ImGui::BeginTable("LodDistances", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
	{
		ImGui::TableSetupColumn("level");
		ImGui::TableSetupColumn("up to distance (world units)");
		ImGui::TableSetupColumn("x scale");
		ImGui::TableHeadersRow();
		for (size_t level = 0; level < distances.size(); ++level)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::Text("LOD%zu", level);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(120.0f);
			const float minimum = level == 0 ? 0.0f : distances[level - 1];
			char id[16];
			std::snprintf(id, sizeof(id), "##lod%zu", level);
			if (ImGui::DragFloat(id, &distances[level], 1.0f, minimum, 100000.0f, "%.1f"))
			{
				distances[level] = std::max(distances[level], minimum);
				distancesChanged = true;
			}
			ImGui::TableNextColumn(); ImGui::Text("%.1f", distances[level] * appliedScale);
		}
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); ImGui::TextUnformatted("LOD4");
		ImGui::TableNextColumn(); ImGui::TextDisabled("beyond LOD3");
		ImGui::TableNextColumn(); ImGui::TextDisabled("-");
		ImGui::EndTable();
	}
	if (distancesChanged)
	{
		// A drag below a later threshold raises the later ones, so the list stays non-decreasing.
		for (size_t level = 1; level < distances.size(); ++level)
		{
			distances[level] = std::max(distances[level], distances[level - 1]);
		}
		chunks->SetLodDistances(distances);
	}
	if (ImGui::Button("Reset LOD settings"))
	{
		chunks->SetLodEnabled(true);
		chunks->SetLodScale(1.0f);
		chunks->ResetLodDistances();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("Distances are measured on the XZ plane from the camera to the chunk centre.");

	// --- Culling ---
	ImGui::SeparatorText("Frustum culling (terrain chunks, main pass)");
	int cullingMode = chunks->IsCullingFrozen() ? 2 : (chunks->IsCullingEnabled() ? 0 : 1);
	bool cullingChanged = ImGui::RadioButton("On", &cullingMode, 0);
	ImGui::SameLine();
	cullingChanged |= ImGui::RadioButton("Off", &cullingMode, 1);
	ImGui::SameLine();
	cullingChanged |= ImGui::RadioButton("Freeze", &cullingMode, 2);
	if (cullingChanged)
	{
		chunks->SetCullingEnabled(cullingMode != 1);
		chunks->SetCullingFrozen(cullingMode == 2);
	}
	ImGui::TextDisabled("Freeze keeps the frustum of the last frame, so the camera can leave it. The shadow pass never culls.");

	// --- Overlays ---
	ImGui::SeparatorText("Overlays");
	bool tint = chunks->IsLodTintEnabled();
	if (ImGui::Checkbox("LOD tint", &tint))
	{
		chunks->SetLodTintEnabled(tint);
	}
	ImGui::SameLine();
	static const ImVec4 lodColors[5] = {		// same colours as terrain.frag
		ImVec4(0.90f, 0.15f, 0.15f, 1.0f), ImVec4(0.95f, 0.60f, 0.10f, 1.0f), ImVec4(0.90f, 0.90f, 0.15f, 1.0f),
		ImVec4(0.20f, 0.75f, 0.30f, 1.0f), ImVec4(0.20f, 0.45f, 0.95f, 1.0f) };
	for (int level = 0; level < 5; ++level)
	{
		ImGui::TextColored(lodColors[level], "LOD%d", level);
		if (level < 4)
		{
			ImGui::SameLine();
		}
	}
	bool borders = chunks->AreChunkBordersEnabled();
	if (ImGui::Checkbox("Chunk borders", &borders))
	{
		chunks->SetChunkBordersEnabled(borders);
	}
	ImGui::TextDisabled("Overlays are drawn in the lit terrain only (not in the terrain debug modes), and appear in screenshots.");
}

void pwg::debug::DebugView::DrawRenderTab(ChunkManager* chunks)
{
	DrawChunkLodSection(chunks);
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
	ImGui::BeginDisabled(m_recorder.IsActive());	// Clear would drop pending events of a CSV window.
	if (ImGui::SmallButton("Clear"))
	{
		m_sim.Clear();
	}
	ImGui::EndDisabled();
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

// --- Capture tab (D-07 screenshot; D-08 adds the mask export) ---

namespace
{
	/** @brief Path from UTF-8 text typed into ImGui (path(std::string) would use the ANSI code page). */
	std::filesystem::path PathFromUtf8(const char* text)
	{
		const std::string value(text);
		return std::filesystem::path(std::u8string(value.begin(), value.end()));
	}
}

void pwg::debug::DebugView::DrawExperimentTab()
{
	if (!m_runner)
	{
		ImGui::TextDisabled("The experiment runner is not available.");
		return;
	}
	const ExperimentRunner::Status status = m_runner->GetStatus();
	const bool running = status.state == ExperimentRunner::State::RUNNING;

	ImGui::TextWrapped("Runs a YAML experiment config (design section 8): load the project, apply the parameters, prepare, "
		"warm up, measure and export, once for every variant and repetition. During a run the editor is driven by the runner; "
		"in the fullscreen main view this panel is hidden, and F9 aborts the run.");
	ImGui::BeginDisabled(running);
	ImGui::SetNextItemWidth(-150.0f);
	ImGui::InputText("Config file##experiment", m_experimentConfig, sizeof(m_experimentConfig));
	ImGui::SetNextItemWidth(-150.0f);
	ImGui::InputText("Output (optional)##experiment", m_experimentOut, sizeof(m_experimentOut));
	if (ImGui::Button("Start"))
	{
		std::string error;
		if (m_experimentConfig[0] == '\0')
		{
			m_experimentMessage = "Enter the path of a config file.";
		}
		else if (m_runner->Start(PathFromUtf8(m_experimentConfig), PathFromUtf8(m_experimentOut), false, error))
		{
			m_experimentMessage.clear();
		}
		else
		{
			m_experimentMessage = error;
		}
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(!running);
	if (ImGui::Button("Abort"))
	{
		m_runner->Abort();
	}
	ImGui::EndDisabled();
	if (!m_experimentMessage.empty())
	{
		ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.40f, 1.0f), "%s", m_experimentMessage.c_str());
	}

	ImGui::SeparatorText("State");
	const char* stateName = "idle";
	switch (status.state)
	{
	case ExperimentRunner::State::RUNNING: stateName = "running"; break;
	case ExperimentRunner::State::DONE: stateName = "done"; break;
	case ExperimentRunner::State::ABORTED: stateName = "aborted"; break;
	case ExperimentRunner::State::REJECTED: stateName = "config rejected"; break;
	default: break;
	}
	ImGui::Text("State: %s", stateName);
	if (!status.experimentId.empty())
	{
		ImGui::Text("Experiment: %s", status.experimentId.c_str());
		ImGui::Text("Units: %d of %d, %d with problems", running ? status.unit : status.unitsTotal, status.unitsTotal, status.unitsFailed);
	}
	if (running)
	{
		ImGui::Text("Variant: %s, repetition %d", status.variant.c_str(), status.repetition);
		if (status.phaseFrames > 0)
		{
			ImGui::Text("Phase: %s, frame %llu of %llu", status.phase.c_str(),
				static_cast<unsigned long long>(status.phaseFrame), static_cast<unsigned long long>(status.phaseFrames));
		}
		else
		{
			ImGui::Text("Phase: %s, frame %llu", status.phase.c_str(), static_cast<unsigned long long>(status.phaseFrame));
		}
		ImGui::Text("Run folder: %s", Utf8(status.unitFolder).c_str());
	}
	else if (!status.outputFolder.empty())
	{
		ImGui::Text("Output folder: %s", Utf8(status.outputFolder).c_str());
	}
	if (!status.message.empty())
	{
		ImGui::TextWrapped("%s", status.message.c_str());
	}
}

void pwg::debug::DebugView::DrawCaptureTab(ChunkManager* chunks)
{
	ImGui::SetNextItemWidth(260.0f);
	ImGui::InputText("Output folder##capture", m_captureRoot, sizeof(m_captureRoot));
	std::error_code error;
	const std::filesystem::path root = std::filesystem::absolute(PathFromUtf8(m_captureRoot), error);
	ImGui::TextDisabled("Screenshots go to <folder>/%s/, masks to <folder>/%s/. Folder: %s", ScreenshotFolderName,
		MaskFolderName, error ? m_captureRoot : Utf8(root).c_str());

	DrawScreenshotSection();
	DrawMaskSection(chunks);
	DrawCameraPathSection();
}

namespace
{
	std::string LowerAscii(std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(),
			[](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
		return text;
	}
}

void pwg::debug::DebugView::DrawMaskSection(ChunkManager* chunks)
{
	ImGui::SeparatorText("Mask export");
	if (!chunks)
	{
		ImGui::TextDisabled("No terrain is loaded.");
		return;
	}
	const bool recording = m_recorder.IsActive();
	auto& masks = chunks->GetMasks();

	std::vector<std::string> names;
	names.reserve(masks.size());
	for (const auto& [name, mask] : masks)
	{
		if (mask && mask->texture)
		{
			names.push_back(name);
		}
	}
	std::sort(names.begin(), names.end());
	// Masks can disappear (project reload, graph change); forget ticks that no longer match a mask.
	std::erase_if(m_maskSelection, [&](const std::string& name) { return !std::binary_search(names.begin(), names.end(), name); });

	ImGui::SetNextItemWidth(200.0f);
	ImGui::InputText("Filter##masks", m_maskFilter, sizeof(m_maskFilter));
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.0f);
	ImGui::InputText("Tag (optional)##masks", m_maskTag, sizeof(m_maskTag));
	ImGui::SameLine();
	ImGui::Checkbox("PNG preview", &m_maskPreview);

	const std::string filter = LowerAscii(m_maskFilter);
	std::vector<const std::string*> shown;
	for (const std::string& name : names)
	{
		if (filter.empty() || LowerAscii(name).find(filter) != std::string::npos)
		{
			shown.push_back(&name);
		}
	}
	if (ImGui::SmallButton("Tick shown"))
	{
		for (const std::string* name : shown)
		{
			if (std::find(m_maskSelection.begin(), m_maskSelection.end(), *name) == m_maskSelection.end())
			{
				m_maskSelection.push_back(*name);
			}
		}
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Untick all"))
	{
		m_maskSelection.clear();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("%zu masks, %zu shown, %zu ticked", names.size(), shown.size(), m_maskSelection.size());

	if (ImGui::BeginChild("##maskList", ImVec2(0.0f, 180.0f), ImGuiChildFlags_Borders))
	{
		for (const std::string* name : shown)
		{
			const auto selected = std::find(m_maskSelection.begin(), m_maskSelection.end(), *name);
			bool ticked = selected != m_maskSelection.end();
			if (ImGui::Checkbox(name->c_str(), &ticked))
			{
				if (ticked)
				{
					m_maskSelection.push_back(*name);
				}
				else
				{
					m_maskSelection.erase(selected);
				}
			}
			const std::shared_ptr<Texture>& texture = masks.at(*name)->texture;
			ImGui::SameLine();
			ImGui::TextDisabled("%dx%d", texture->GetWidth(), texture->GetHeight());
		}
	}
	ImGui::EndChild();

	ImGui::BeginDisabled(recording || m_maskSelection.empty());
	char label[64];
	std::snprintf(label, sizeof(label), "Export ticked masks (%zu)", m_maskSelection.size());
	if (ImGui::Button(label))
	{
		MaskExportRequest request;
		request.root = PathFromUtf8(m_captureRoot);
		request.names = m_maskSelection;
		std::sort(request.names.begin(), request.names.end());
		request.tag = m_maskTag;
		request.writePreview = m_maskPreview;
		ExportMasks(*chunks, request);
	}
	ImGui::EndDisabled();
	if (recording)
	{
		ImGui::SameLine();
		ImGui::TextDisabled("Not available while a CSV window is recorded (synchronous readback).");
	}
	ImGui::TextDisabled("Writes <name>[_<tag>].f32 (raw float32, GL row 0 first), a .yaml sidecar and an 8-bit .png "
		"preview, then reads the .f32 back and compares it byte for byte. Runs at once and blocks the frame.");

	if (m_exporters.HasMaskExportResult())
	{
		const std::vector<MaskExportResult>& results = m_exporters.GetLastMaskExport();
		const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;
		if (ImGui::BeginTable("##maskResults", 6, flags))
		{
			ImGui::TableSetupColumn("Mask");
			ImGui::TableSetupColumn("Size");
			ImGui::TableSetupColumn("Min");
			ImGui::TableSetupColumn("Max");
			ImGui::TableSetupColumn("Round trip");
			ImGui::TableSetupColumn("ms (read / raw / verify / png)");
			ImGui::TableHeadersRow();
			for (const MaskExportResult& result : results)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::TextUnformatted(result.name.c_str());
				if (!result.error.empty())
				{
					ImGui::TableNextColumn();
					ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "failed: %s", result.error.c_str());
					continue;
				}
				ImGui::TableNextColumn(); ImGui::Text("%dx%dx%d", result.width, result.height, result.channels);
				ImGui::TableNextColumn(); ImGui::Text("%.6g", static_cast<double>(result.min));
				ImGui::TableNextColumn(); ImGui::Text("%.6g", static_cast<double>(result.max));
				ImGui::TableNextColumn(); ImGui::TextUnformatted(result.roundTripIdentical ? "identical" : "DIFFERS");
				ImGui::TableNextColumn(); ImGui::Text("%.1f / %.1f / %.1f / %.1f", result.readbackMs, result.rawWriteMs,
					result.verifyMs, result.previewMs);
			}
			ImGui::EndTable();
		}
		if (!results.empty() && results.front().written)
		{
			const std::string folder = Utf8(results.front().raw.parent_path());
			ImGui::TextWrapped("%s", folder.c_str());
			ImGui::SameLine();
			if (ImGui::SmallButton("Copy path##masks"))
			{
				ImGui::SetClipboardText(folder.c_str());
			}
		}
	}
}

void pwg::debug::DebugView::DrawScreenshotSection()
{
	ImGui::SeparatorText("Screenshot");
	const bool recording = m_recorder.IsActive();
	const bool pending = m_exporters.HasPendingScreenshot();

	ImGui::SetNextItemWidth(200.0f);
	ImGui::InputText("Tag (optional)", m_screenshotTag, sizeof(m_screenshotTag));

	ImGui::BeginDisabled(recording || pending);
	if (ImGui::Button("Screenshot"))
	{
		ScreenshotRequest request;
		request.root = PathFromUtf8(m_captureRoot);
		request.tag = m_screenshotTag;
		RequestScreenshot(std::move(request));
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (recording)
	{
		ImGui::TextDisabled("Not available while a CSV window is recorded (synchronous readback).");
	}
	else if (pending)
	{
		ImGui::TextUnformatted("Taken at the next frame...");
	}
	ImGui::TextDisabled("Saves the Main View after post-processing, without UI, at the current render size "
		"(benchmark mode: the fixed render size). Writes <project>_<camera>_<time>[_<tag>].png and a .yaml sidecar.");

	if (m_exporters.HasScreenshotResult())
	{
		const ScreenshotResult& result = m_exporters.GetLastScreenshot();
		if (result.written)
		{
			ImGui::Text("Last screenshot: %dx%d, readback %.2f ms, PNG %.2f ms", result.width, result.height,
				result.readbackMs, result.writeMs);
			const std::string file = Utf8(result.png);
			ImGui::TextWrapped("%s", file.c_str());
			ImGui::SameLine();
			if (ImGui::SmallButton("Copy path##screenshot"))
			{
				ImGui::SetClipboardText(file.c_str());
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Last screenshot failed: %s", result.error.c_str());
		}
	}
}

namespace
{
	/** @brief File name part of a camera path: ASCII letters, digits, '.', '-' and '_' are kept, others become '-'. */
	std::string PathFileStem(const char* text)
	{
		std::string stem;
		for (const char* c = text; *c != '\0'; ++c)
		{
			const unsigned char ch = static_cast<unsigned char>(*c);
			stem += (std::isalnum(ch) || ch == '.' || ch == '-' || ch == '_') ? static_cast<char>(ch) : '-';
		}
		return stem.empty() ? std::string("flyover_path") : stem;
	}
}

void pwg::debug::DebugView::DrawCameraPathSection()
{
	ImGui::SeparatorText("Camera path (flyover camera)");
	const bool recording = m_cameraPath.IsRecording();
	const bool playing = m_cameraPath.IsPlaying();
	const bool idle = !recording && !playing;

	const std::filesystem::path folder = PathFromUtf8(m_captureRoot) / CameraPath::FolderName;
	const std::string stem = PathFileStem(m_pathName);
	const std::filesystem::path file = folder / (stem + CameraPath::FileExtension);

	ImGui::BeginDisabled(!idle);
	ImGui::SetNextItemWidth(200.0f);
	ImGui::InputText("Name##path", m_pathName, sizeof(m_pathName));
	ImGui::SameLine();
	ImGui::SetNextItemWidth(110.0f);
	ImGui::InputInt("Steps per second##path", &m_pathStepsPerSecond, 0, 0);
	m_pathStepsPerSecond = std::clamp(m_pathStepsPerSecond, 1, 1000);
	ImGui::SameLine();
	ImGui::Checkbox("Matrix dump##path", &m_pathMatrixDump);
	ImGui::EndDisabled();
	ImGui::TextDisabled("File: %s", Utf8(file).c_str());

	// Record / stop
	if (recording)
	{
		if (ImGui::Button("Stop recording##path"))
		{
			m_cameraPath.StopRecording();
			m_pathMessage.clear();
		}
	}
	else
	{
		ImGui::BeginDisabled(!idle);
		if (ImGui::Button("Record##path"))
		{
			m_cameraPath.StartRecording();
			m_pathMessage.clear();
		}
		ImGui::EndDisabled();
	}
	ImGui::SameLine();

	// Play / stop
	if (playing)
	{
		if (ImGui::Button("Stop playback##path"))
		{
			m_cameraPath.StopPlayback();
		}
	}
	else
	{
		ImGui::BeginDisabled(!idle || !m_cameraPath.HasPath());
		if (ImGui::Button("Play##path"))
		{
			CameraPathPlayback options;
			options.stepSeconds = 1.0 / static_cast<double>(m_pathStepsPerSecond);
			if (m_pathMatrixDump)
			{
				const std::string dumpStem = UniqueStem(folder, stem + "_" + FormatFileTimestamp(std::chrono::system_clock::now()),
					std::string::npos, { ".matrices.txt" });
				options.matrixDumpFile = folder / (dumpStem + ".matrices.txt");
			}
			std::string error;
			m_pathMessageIsError = !m_cameraPath.StartPlayback(options, error);
			m_pathMessage = m_pathMessageIsError ? "Play: " + error : std::string();
		}
		ImGui::EndDisabled();
	}
	ImGui::SameLine();

	ImGui::BeginDisabled(!idle || m_cameraPath.GetKeyCount() == 0);
	if (ImGui::Button("Save##path"))
	{
		std::string error;
		m_pathMessageIsError = !m_cameraPath.Save(file, m_pathOverwrite, error);
		m_pathMessage = m_pathMessageIsError ? "Save: " + error : "Saved " + Utf8(file);
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(!idle);
	ImGui::Checkbox("Overwrite##path", &m_pathOverwrite);
	ImGui::SameLine();
	if (ImGui::Button("Load##path"))
	{
		std::string error;
		m_pathMessageIsError = !m_cameraPath.Load(file, error);
		m_pathMessage = m_pathMessageIsError ? "Load: " + error : "Loaded " + Utf8(file);
	}
	ImGui::EndDisabled();

	// Status
	if (recording)
	{
		if (m_cameraPath.GetKeyCount() == 0)
		{
			ImGui::TextUnformatted("Recording: waiting for the flyover camera (F2). Samples are taken only while it is active.");
		}
		else
		{
			ImGui::Text("Recording: %zu keys, %.2f s. Press F1 (editor camera) to stop; the flyover camera captures the mouse.",
				m_cameraPath.GetKeyCount(), m_cameraPath.GetDuration());
		}
	}
	else if (playing)
	{
		ImGui::Text("Playing: frame %lld of %llu at %.6f s per frame. The mouse is released; F1 or F3 ends it.",
			static_cast<long long>(m_cameraPath.GetAppliedFrame()), static_cast<unsigned long long>(m_cameraPath.GetFrameCount()),
			m_cameraPath.GetStep());
	}
	else if (m_cameraPath.GetKeyCount() > 0)
	{
		const double step = 1.0 / static_cast<double>(m_pathStepsPerSecond);
		ImGui::Text("Path: %zu keys, %.3f s, %llu frames at %d steps per second", m_cameraPath.GetKeyCount(),
			m_cameraPath.GetDuration(),
			static_cast<unsigned long long>(CameraPath::FrameCount(m_cameraPath.GetDuration(), step)), m_pathStepsPerSecond);
		ImGui::TextDisabled("%s", m_cameraPath.GetFile().empty() ? "not saved" : Utf8(m_cameraPath.GetFile()).c_str());
		ImGui::TextDisabled("sha256 %s", m_cameraPath.GetSha256().c_str());
	}
	else
	{
		ImGui::TextDisabled("No path. Record one while flying with F2, or load a file.");
	}
	if (!m_pathMessage.empty())
	{
		if (m_pathMessageIsError)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", m_pathMessage.c_str());
		}
		else
		{
			ImGui::TextUnformatted(m_pathMessage.c_str());
		}
	}

	if (m_cameraPath.HasDumpResult() && idle)
	{
		const MatrixDumpResult& dump = m_cameraPath.GetLastDump();
		if (dump.written)
		{
			ImGui::Text("Matrix dump: %llu frames, sha256 %s", static_cast<unsigned long long>(dump.frames), dump.sha256.c_str());
			ImGui::TextWrapped("%s", Utf8(dump.file).c_str());
			ImGui::SameLine();
			if (ImGui::SmallButton("Copy path##dump"))
			{
				ImGui::SetClipboardText(Utf8(dump.file).c_str());
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Matrix dump failed: %s", dump.error.c_str());
		}
	}
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
	ImGui::TextWrapped("Only the flyover camera is recorded: one key per frame (time, position, yaw, pitch, roll). Playback is "
		"indexed by frame (t = frame x step), not by wall time, and replaces the camera update. The matrix dump lists the view "
		"and projection matrix of every path frame as hex floats; two playbacks at the same render size give identical dumps. "
		"Switch the matrix dump off for measured runs (it keeps the rows in memory until the playback ends).");
	ImGui::PopStyleColor();
}

// ---------------------------------------------------------------------------------------------------------------------
// Memory tab (D-04, design 5.7)
// ---------------------------------------------------------------------------------------------------------------------

namespace
{
	constexpr double MemoryRefreshSeconds = 0.25;

	/** @brief One table row: name, size in MiB, exact bytes, source label, method. `bytes` null = not available. */
	void MemoryRow(const char* name, const uint64_t* bytes, const char* source, const char* method)
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); ImGui::TextUnformatted(name);
		ImGui::TableNextColumn();
		if (bytes)
		{
			ImGui::Text("%.2f", static_cast<double>(*bytes) / (1024.0 * 1024.0));
		}
		else
		{
			ImGui::TextDisabled("-");
		}
		ImGui::TableNextColumn();
		if (bytes)
		{
			ImGui::Text("%llu", static_cast<unsigned long long>(*bytes));
		}
		else
		{
			ImGui::TextDisabled("-");
		}
		ImGui::TableNextColumn(); ImGui::TextUnformatted(source);
		ImGui::TableNextColumn(); ImGui::TextUnformatted(method);
	}

	bool BeginMemoryTable(const char* id)
	{
		const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;
		if (!ImGui::BeginTable(id, 5, flags))
		{
			return false;
		}
		ImGui::TableSetupColumn("value");
		ImGui::TableSetupColumn("MiB");
		ImGui::TableSetupColumn("bytes");
		ImGui::TableSetupColumn("source label");
		ImGui::TableSetupColumn("method");
		ImGui::TableHeadersRow();
		return true;
	}
}

void pwg::debug::DebugView::DrawMemoryTab()
{
	if constexpr (!InstrumentationCompiled)
	{
		ImGui::TextDisabled("Instrumentation compiled out (PWG_DEBUG_VIEW_INSTRUMENTATION=OFF). No memory statistics.");
		return;
	}

	// Own accounting is a set of atomics and is read every frame. The process and VRAM queries (a system call and two
	// glGetIntegerv) are repeated a few times per second, only while this tab is drawn.
	const double now = ImGui::GetTime();
	if (m_memoryShownTime < 0.0 || now - m_memoryShownTime >= MemoryRefreshSeconds || now < m_memoryShownTime)
	{
		m_memoryShown = MemoryStats::TakeSnapshot();
		m_memoryShownTime = now;
	}
	else
	{
		for (size_t i = 0; i < MemoryCategoryCount; ++i)
		{
			m_memoryShown.own[i] = MemoryStats::GetTotals(static_cast<MemoryCategory>(i));
		}
	}
	const MemorySnapshot& snapshot = m_memoryShown;

	ImGui::TextWrapped("Three separate numbers from three methods. They are never added to or subtracted from each other: own accounting counts "
		"only the allocations listed below, process memory is the CPU side, and driver VRAM is the whole GPU.");
	ImGui::Spacing();

	// --- Own accounting ---
	ImGui::SeparatorText("Own accounting  [own_accounting]");
	if (BeginMemoryTable("MemoryOwn"))
	{
		static const char* const Methods[MemoryCategoryCount] = {
			"width x height x bytes per texel of the internal format, plus the mip chain if mipmaps were generated (pwg::Texture)",
			"particle capacity x 32 B (GpuParticleErosion)",
			"world size^2 x 4 B per slot (ChunkManager height readback)",
			"capacity of the instance buffer (Mesh::DrawInstanced)",
		};
		static const char* const Names[MemoryCategoryCount] = {
			"textures (pwg::Texture)", "particle SSBO", "height readback PBOs", "instance VBOs",
		};
		for (size_t i = 0; i < MemoryCategoryCount; ++i)
		{
			MemoryRow(Names[i], &snapshot.own[i].liveBytes, "own_accounting", Methods[i]);
		}
		const uint64_t total = snapshot.OwnLiveBytes();
		MemoryRow("total of the four (live)", &total, "own_accounting", "sum of the rows above");
		ImGui::EndTable();
	}
	const MemoryCategoryTotals& textures = snapshot.own[static_cast<size_t>(MemoryCategory::TEXTURE)];
	ImGui::Text("Texture objects alive: %llu (peak %.2f MiB)  |  destroyed textures whose GL texture was not deleted: %llu, %.2f MiB",
		static_cast<unsigned long long>(textures.liveObjects),
		static_cast<double>(textures.peakBytes) / (1024.0 * 1024.0),
		static_cast<unsigned long long>(textures.orphanedObjects),
		static_cast<double>(textures.orphanedBytes) / (1024.0 * 1024.0));
	ImGui::TextDisabled("The engine's ~Texture does not call glDeleteTextures, so a destroyed pwg::Texture leaves its storage in the driver. "
		"The live rows drop when the object is destroyed; the line above counts the bytes that stayed.");
	ImGui::TextWrapped("Own accounting covers ONLY these allocations: pwg::Texture objects, the particle erosion SSBO, the height readback PBOs and "
		"the mesh instance buffers. It does NOT cover mesh vertex and index buffers (terrain chunks, vegetation), framebuffers and their depth "
		"textures, shadow maps, cube maps and texture arrays, the erosion and water textures created outside pwg::Texture, or the driver's own "
		"padding and bookkeeping. Sizes are nominal (as requested), not measured in the driver.");

	// --- Process memory ---
	ImGui::SeparatorText("Process memory  [os_process]");
	if (BeginMemoryTable("MemoryProcess"))
	{
		const ProcessMemory& process = snapshot.process;
		MemoryRow("private usage", process.available ? &process.privateBytes : nullptr, "os_process", "GetProcessMemoryInfo, PrivateUsage");
		MemoryRow("working set", process.available ? &process.workingSetBytes : nullptr, "os_process", "GetProcessMemoryInfo, WorkingSetSize");
		MemoryRow("peak working set", process.available ? &process.peakWorkingSetBytes : nullptr, "os_process", "GetProcessMemoryInfo, PeakWorkingSetSize");
		ImGui::EndTable();
	}
	ImGui::TextDisabled("CPU-side memory of this process (heap, mapped files, driver user-mode allocations). Not GPU memory.");

	// --- VRAM ---
	const GpuMemory& gpu = snapshot.gpu;
	ImGui::SeparatorText(gpu.available ? "Video memory  [driver_nvx]" : "Video memory  [unavailable]");
	if (gpu.available)
	{
		if (BeginMemoryTable("MemoryGpu"))
		{
			MemoryRow("dedicated", &gpu.dedicatedBytes, "driver_nvx", "GL_NVX_gpu_memory_info, DEDICATED_VIDMEM_NVX");
			MemoryRow("currently available", &gpu.availableBytes, "driver_nvx", "GL_NVX_gpu_memory_info, CURRENT_AVAILABLE_VIDMEM_NVX");
			MemoryRow("used (dedicated - available)", &gpu.usedBytes, "driver_nvx", "dedicated - currently available");
			ImGui::EndTable();
		}
		ImGui::TextDisabled("Used VRAM is reported by the driver for the whole GPU: this process, other processes (the desktop, the browser) and "
			"the driver's own use. It is shown next to own accounting and never replaces it.");
	}
	else
	{
		ImGui::TextWrapped("GL_NVX_gpu_memory_info is not available on this GPU or driver, so no VRAM value is shown (the CSV rows carry source 'unavailable' "
			"and an empty value). Own accounting and process memory are still valid.");
	}

	// --- CSV ---
	ImGui::SeparatorText("CSV");
	ImGui::BeginDisabled(m_recorder.GetState() != CsvRecorder::State::RECORDING);
	if (ImGui::Button("Add snapshot to the CSV window"))
	{
		RecordMemorySnapshot("manual");
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("Reset peaks"))
	{
		MemoryStats::ResetPeaks();
	}
	ImGui::TextDisabled("memory_snapshot rows (events.csv): metric bytes, unit B, source = the label of the table. The runner adds them with memory_snapshots: true.");
	if (!IsEnabled())
	{
		ImGui::TextDisabled("Own accounting counts whether the Debug View is enabled or not.");
	}
}
