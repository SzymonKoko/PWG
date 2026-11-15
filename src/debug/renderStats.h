#ifndef SRC_DEBUG_RENDERSTATS_H_
#define SRC_DEBUG_RENDERSTATS_H_

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include "debug/frameStats.h"

namespace pwg::debug
{
	/**
	* @brief Draw commands and submitted triangles of one counter slot (a GpuPass or "other").
	*/
	struct PassCounters
	{
		uint32_t drawCalls{ 0 };
		uint64_t triangles{ 0 };
	};

	/**
	* @brief Shadow pass counters of one cascade.
	*/
	struct CascadeCounters
	{
		uint32_t drawCalls{ 0 };
		uint64_t triangles{ 0 };
		uint32_t chunks{ 0 };						/**< Terrain chunks drawn (all chunks, no culling in the shadow pass). */
		uint64_t vegetationInstances{ 0 };			/**< Shadow-casting tree instances drawn. */
	};

	/**
	* @brief Vegetation counters of one model, indexed by the VegetationSystem model index.
	*/
	struct VegetationModelCounters
	{
		std::string name;
		uint64_t visible{ 0 };						/**< Main pass: instances drawn. */
		uint64_t culled{ 0 };						/**< Main pass: candidates of this model minus instances drawn. */
		uint64_t shadow{ 0 };						/**< Shadow pass: instances drawn, summed over cascades. */
		bool counted{ false };						/**< The model reached Draw or DrawShadow this frame. */
	};

	/**
	* @brief All render counters of one frame (design section 5.5).
	*/
	struct RenderCounters
	{
		std::array<PassCounters, CounterPassCount> passes{};
		std::array<double, GpuPassCount> cpuPassMs{};				/**< CPU submission time per pass scope, NotAvailable if not scoped. */
		std::array<CascadeCounters, ShadowCascadeCount> cascades{};
		std::array<uint32_t, TerrainLodCount> chunksVisible{};
		uint32_t chunkCount{ 0 };
		bool chunksCounted{ false };								/**< DrawTerrainChunks ran this frame. */
		uint64_t vegetationVisible{ 0 };
		uint64_t vegetationCulled{ 0 };
		uint64_t vegetationShadow{ 0 };
		std::vector<VegetationModelCounters> models;				/**< Names keep their capacity across frames. */

		void Reset()
		{
			passes.fill(PassCounters{});
			cpuPassMs.fill(NotAvailable);
			cascades.fill(CascadeCounters{});
			chunksVisible.fill(0);
			chunkCount = 0;
			chunksCounted = false;
			vegetationVisible = 0;
			vegetationCulled = 0;
			vegetationShadow = 0;
			for (VegetationModelCounters& model : models)
			{
				model.visible = 0;
				model.culled = 0;
				model.shadow = 0;
				model.counted = false;
			}
		}

		uint32_t GetVisibleChunks() const
		{
			uint32_t sum = 0;
			for (uint32_t count : chunksVisible)
			{
				sum += count;
			}
			return sum;
		}
	};

	/**
	* @brief Per-frame render counters, attributed to the pass that is active when they are counted.
	*
	* Counters are plain CPU integers, written on the main thread only and never inside an OpenMP region.
	* They are reset at frame begin. At frame end a frame counted from begin to end becomes the "last frame"
	* shown in the Render tab, and its totals are copied into the frame record. Instrumentation points call
	* it through PWG_RENDER_COUNT, so with the Debug View disabled each point costs one branch, and with
	* PWG_DEBUG_VIEW_INSTRUMENTATION=OFF nothing.
	*/
	class RenderStats
	{
	public:
		using Clock = std::chrono::steady_clock;

		RenderStats() { m_current.Reset(); m_last.Reset(); }

		/** @brief Resets the counters. `counting` is whether the Debug View is enabled at frame begin. */
		void BeginFrame(uint64_t frameIndex, bool counting)
		{
			m_current.Reset();
			m_frameIndex = frameIndex;
			m_frameCounting = counting;
			m_activePass = OtherCounterPass;
			m_activeCascade = -1;
		}

		/**
		* @brief Publishes the frame if counting was on at its begin and end; a frame in which the Debug View
		* was switched on or off is not complete and is not published.
		*/
		void EndFrame(bool counting, FrameRecord* record)
		{
			if (!m_frameCounting || !counting)
			{
				return;
			}
			if (record)
			{
				WriteRecord(*record);
			}
			std::swap(m_current, m_last);
			m_lastFrameIndex = m_frameIndex;
			m_hasLast = true;
		}

		/** @brief Makes `pass` the counter slot of following draws. Returns the slot to restore at EndPass. */
		size_t BeginPass(GpuPass pass)
		{
			const size_t previous = m_activePass;
			m_activePass = static_cast<size_t>(pass) < GpuPassCount ? static_cast<size_t>(pass) : OtherCounterPass;
			return previous;
		}

		void EndPass(GpuPass pass, size_t previous, Clock::duration cpuTime)
		{
			const size_t index = static_cast<size_t>(pass);
			if (index < GpuPassCount)
			{
				const double ms = std::chrono::duration<double, std::milli>(cpuTime).count();
				double& value = m_current.cpuPassMs[index];
				value = std::isnan(value) ? ms : value + ms;
			}
			m_activePass = previous;
		}

		/** @brief Shadow cascade of following draws, or -1 outside the cascade loop. */
		void SetCascade(int cascade) { m_activeCascade = cascade; }

		/** @brief One GL draw command with `triangles` submitted triangles (all instances). */
		void AddDraw(uint64_t triangles)
		{
			PassCounters& pass = m_current.passes[m_activePass];
			++pass.drawCalls;
			pass.triangles += triangles;
			if (CascadeCounters* cascade = ActiveCascade())
			{
				++cascade->drawCalls;
				cascade->triangles += triangles;
			}
		}

		/** @brief Main pass: total chunk count, set once per DrawTerrainChunks call. */
		void SetChunkCount(uint32_t count)
		{
			m_current.chunkCount = count;
			m_current.chunksCounted = true;
		}

		/** @brief Main pass: a chunk passed IsChunkVisible and is drawn at `lod`. */
		void AddVisibleChunk(int lod)
		{
			if (lod >= 0 && static_cast<size_t>(lod) < TerrainLodCount)
			{
				++m_current.chunksVisible[static_cast<size_t>(lod)];
			}
		}

		/** @brief Shadow pass: a chunk drawn into the active cascade. */
		void AddShadowChunk()
		{
			if (CascadeCounters* cascade = ActiveCascade())
			{
				++cascade->chunks;
			}
		}

		/** @brief Main pass: `visible` of `candidates` instances of a model were drawn. */
		void AddVegetation(int modelIndex, const std::string& name, uint64_t visible, uint64_t candidates)
		{
			const uint64_t culled = candidates > visible ? candidates - visible : 0;
			m_current.vegetationVisible += visible;
			m_current.vegetationCulled += culled;
			if (VegetationModelCounters* model = Model(modelIndex, name))
			{
				model->visible += visible;
				model->culled += culled;
			}
		}

		/** @brief Shadow pass: `instances` instances of a model were drawn into the active cascade. */
		void AddVegetationShadow(int modelIndex, const std::string& name, uint64_t instances)
		{
			m_current.vegetationShadow += instances;
			if (CascadeCounters* cascade = ActiveCascade())
			{
				cascade->vegetationInstances += instances;
			}
			if (VegetationModelCounters* model = Model(modelIndex, name))
			{
				model->shadow += instances;
			}
		}

		/** @brief Counters of the last frame that was counted from begin to end. */
		const RenderCounters& GetLastFrame() const { return m_last; }
		bool HasLastFrame() const { return m_hasLast; }
		uint64_t GetLastFrameIndex() const { return m_lastFrameIndex; }

	private:
		CascadeCounters* ActiveCascade()
		{
			if (m_activeCascade < 0 || static_cast<size_t>(m_activeCascade) >= ShadowCascadeCount)
			{
				return nullptr;
			}
			return &m_current.cascades[static_cast<size_t>(m_activeCascade)];
		}

		VegetationModelCounters* Model(int modelIndex, const std::string& name)
		{
			if (modelIndex < 0)
			{
				return nullptr;
			}
			const size_t index = static_cast<size_t>(modelIndex);
			if (index >= m_current.models.size())
			{
				m_current.models.resize(index + 1);
			}
			VegetationModelCounters& model = m_current.models[index];
			if (!model.counted)
			{
				// Assignment keeps the string's capacity, so a steady scene does not allocate here.
				model.name = name;
				model.counted = true;
			}
			return &model;
		}

		void WriteRecord(FrameRecord& record) const
		{
			record.countersValid = true;
			for (size_t slot = 0; slot < CounterPassCount; ++slot)
			{
				record.drawCalls[slot] = m_current.passes[slot].drawCalls;
				record.triangles[slot] = m_current.passes[slot].triangles;
			}
			record.chunksVisible = m_current.chunksVisible;
			const uint32_t visible = m_current.GetVisibleChunks();
			record.chunksCulled = m_current.chunkCount > visible ? m_current.chunkCount - visible : 0;
			record.vegetationVisible = m_current.vegetationVisible;
			record.vegetationCulled = m_current.vegetationCulled;
			record.vegetationShadow = m_current.vegetationShadow;
		}

		RenderCounters m_current;
		RenderCounters m_last;
		uint64_t m_frameIndex{ 0 };
		uint64_t m_lastFrameIndex{ 0 };
		size_t m_activePass{ OtherCounterPass };
		int m_activeCascade{ -1 };
		bool m_frameCounting{ false };
		bool m_hasLast{ false };
	};
} // namespace pwg::debug

#endif // !SRC_DEBUG_RENDERSTATS_H_
