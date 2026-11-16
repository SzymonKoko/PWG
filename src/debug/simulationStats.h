#ifndef SRC_DEBUG_SIMULATIONSTATS_H_
#define SRC_DEBUG_SIMULATIONSTATS_H_

#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>
#include "debug/frameStats.h"
#include "debug/gpuProfiler.h"

namespace pwg::debug
{
	/**
	* @brief One-shot work timed by the Debug View (design section 5.6). Each type is one row kind of events.csv.
	*/
	enum class SimEventType : uint8_t
	{
		GRAPH_RUN,					/**< ChunkManager::RunUniversalGraph: CPU wall time, includes the synchronous height readback. */
		GRAPH_EXECUTE,				/**< TerrainComputePipeline::ExecuteUniversalGraph: CPU scope and TIMESTAMP span. */
		GRAPH_NODE,					/**< Body of the node loop for one node: CPU submission and TIME_ELAPSED. */
		GRAPH_MASK_PASS,			/**< Texture clears and the mask pass after the node loop. */
		GRAPH_PREVIEWS,				/**< Node preview generation after the node loop. */
		HEIGHT_READBACK_SYNC,		/**< ChunkManager::UpdateHeightCache: synchronous FinalHeight readback. */
		HEIGHT_READBACK_QUEUE,		/**< ChunkManager::QueueHeightCacheReadback: PBO read and fence. */
		HEIGHT_READBACK_PUBLISH,	/**< ChunkManager::PollHeightCacheReadback after the fence signalled: map and copy. */
		HYDRAULIC_STEP,				/**< TerrainComputePipeline::StepParticleErosion; GPU part is GpuParticleErosion::Step. */
		HYDRAULIC_REFRESH,			/**< Mask pass and previews on every fourth hydraulic step. */
		THERMAL_STEP,				/**< TerrainComputePipeline::StepThermalErosion; GPU part is GpuThermalErosion::Step. */
		THERMAL_REFRESH,			/**< Mask pass and previews on every fourth thermal step. */
		THERMAL_CHECK,				/**< Synchronous stability readback inside GpuThermalErosion::Step (every 8th solver step). */
		THERMAL_STABLE,				/**< Marker: the solver step at which thermal erosion became stable. */
		HYDROLOGY_READBACK,			/**< Synchronous FinalHeight readback in TerrainComputePipeline::GenerateHydrology. */
		HYDROLOGY_STAGE,			/**< Worker wall time of one stage of DrainageNetworkGenerator::Generate. */
		HYDROLOGY_WORKER,			/**< Worker wall time of the whole Generate call. */
		HYDROLOGY_END_TO_END,		/**< Start to TryTakeResult on the main thread; resolution of one frame. */
		HYDROLOGY_UPLOAD,			/**< Uploads, graph outputs and mask pass in TerrainComputePipeline::UpdateHydrology. */
		COUNT
	};

	constexpr size_t SimEventTypeCount = static_cast<size_t>(SimEventType::COUNT);

	/** @brief Short event type name used in the UI (and later in events.csv). */
	const char* GetSimEventName(SimEventType type);

	/**
	* @brief One timed piece of one-shot work. Missing values are NaN or -1, never 0.
	*/
	struct SimEvent
	{
		SimEventType type{ SimEventType::COUNT };
		uint64_t frameIndex{ 0 };				/**< Frame in which the event started. */
		uint64_t runIndex{ 0 };					/**< Graph run (graph and readback events inside it) or hydrology run; 0 if none. */
		int64_t stepIndex{ -1 };				/**< Pipeline step counter after the step (drives the refresh every 4th step). */
		int64_t solverStep{ -1 };				/**< Step counter of the erosion object after the step (talus every 8th, thermal check every 8th). */
		int32_t stage{ -1 };					/**< Hydrology stage number. */
		const char* label{ nullptr };			/**< Hydrology stage name (string literal). */
		int32_t nodeId{ -1 };
		std::string nodeType;					/**< Graph node type id. */
		bool nodeEnabled{ true };
		int32_t resolution{ 0 };				/**< Texels per side of the output. */
		int32_t activeParticles{ -1 };
		bool refreshStep{ false };				/**< This step also ran the mask pass and previews. */
		bool talusStep{ false };				/**< This hydraulic step also ran the talus relaxation. */
		bool stable{ false };					/**< Thermal: stable after this step or check. */
		bool success{ true };					/**< Graph execution: false if it returned early. */
		int64_t latencyFrames{ -1 };			/**< Readback publish or hydrology end to end: frames since the start. */
		double cpuMs{ NotAvailable };			/**< steady_clock wall time on the thread that did the work. */
		double gpuMs{ NotAvailable };			/**< GPU time; NaN until the result arrives or if not timed. */
		GpuTimerKind gpuTimer{ GpuTimerKind::NONE };
		uint64_t gpuEvent{ 0 };					/**< Pending GpuProfiler event id, 0 if none. */
	};

	/**
	* @brief Collects the one-shot events of graph execution, simulation steps, hydrology and height readbacks.
	*
	* Main thread only. The hydrology worker never touches it: its stage times travel in DrainageNetworkResult
	* and are submitted here by the main thread after TryTakeResult. An event with a GPU part waits in the
	* pending list until GpuProfiler::HarvestEvents returns its result; only then is it complete. The latest
	* event of each type is visible at once with its GPU time still NaN, so the UI shows CPU times immediately.
	*/
	class SimulationStats
	{
	public:
		using Clock = std::chrono::steady_clock;

		static constexpr uint32_t RecentEventCount = 512;

		SimulationStats();

		/** @brief Frame index used for new events. Called by DebugView at frame begin. */
		void SetFrameIndex(uint64_t frameIndex) { m_frameIndex = frameIndex; }

		/** @brief Fills type, frame and run index of a new event. GRAPH_RUN and GRAPH_EXECUTE open a graph run. */
		void Open(SimEvent& event, SimEventType type);

		/** @brief Takes a finished event. It is complete now if it has no pending GPU part. */
		void Close(SimEvent&& event);

		/** @brief Writes GPU results into the pending events they belong to. */
		void ApplyGpuResults(const std::vector<GpuEventResult>& results);

		/** @brief Marker event: thermal erosion became stable at solver step `solverStep`. No time is attached. */
		void SubmitThermalStable(int64_t solverStep);

		/** @brief Starts a new hydrology run; called before the readback in GenerateHydrology. */
		void BeginHydrologyRun();

		/** @brief Records the time and frame at which the hydrology worker was started. */
		void MarkHydrologyStarted();

		/**
		* @brief Submits the worker timing of a finished hydrology run and its end-to-end time.
		* `stageMs[i]` is stage i + 1; NaN for stages that did not run. Ignores runs without a recorded start.
		*/
		void SubmitHydrologyResult(const double* stageMs, size_t stageCount, const char* const* stageNames,
			double resampleMs, double workerMs);

		/** @brief Forgets the start of a run whose result was cancelled or invalid. */
		void AbortHydrologyRun() { m_hydrologyStartValid = false; }

		/** @brief Clears all records (UI button). GPU results of events still pending are then ignored. */
		void Clear();

		/** @brief Latest event of a type, or nullptr if none was recorded. */
		const SimEvent* GetLatest(SimEventType type) const;

		uint64_t GetLastGraphRun() const { return m_graphRun; }
		uint64_t GetLastHydrologyRun() const { return m_hydrologyRun; }

		/** @brief Node events of the latest graph run in execution order. */
		const std::vector<SimEvent>& GetLastGraphNodes() const { return m_lastGraphNodes; }

		/** @brief Stage events of the latest hydrology run, by stage number - 1. */
		const std::vector<SimEvent>& GetLastHydrologyStages() const { return m_lastHydrologyStages; }

		/** @brief Events of each type recorded since start or the last Clear. */
		uint64_t GetCount(SimEventType type) const { return m_counts[static_cast<size_t>(type)]; }

		size_t GetPendingCount() const { return m_pending.size(); }

		/** @brief Complete events in completion order, oldest first, up to `count`. */
		void CopyRecentCompleted(std::vector<SimEvent>& out, uint32_t count) const;

		/** @brief Complete events since start; with the ring size it shows how many a consumer missed. */
		uint64_t GetCompletedTotal() const { return m_completedTotal; }

	private:
		void Complete(SimEvent&& event);
		void UpdateViews(const SimEvent& event);

		uint64_t m_frameIndex{ 0 };
		uint64_t m_graphRun{ 0 };
		uint32_t m_graphDepth{ 0 };
		uint64_t m_hydrologyRun{ 0 };
		bool m_hydrologyStartValid{ false };
		Clock::time_point m_hydrologyStart{};
		uint64_t m_hydrologyStartFrame{ 0 };

		std::vector<SimEvent> m_pending;
		std::array<SimEvent, SimEventTypeCount> m_latest{};
		std::array<bool, SimEventTypeCount> m_hasLatest{};
		std::array<uint64_t, SimEventTypeCount> m_counts{};
		std::vector<SimEvent> m_lastGraphNodes;
		std::vector<SimEvent> m_lastHydrologyStages;

		std::vector<SimEvent> m_recent;		/**< Ring of complete events. */
		uint32_t m_recentHead{ 0 };
		uint64_t m_completedTotal{ 0 };
	};
} // namespace pwg::debug

#endif // !SRC_DEBUG_SIMULATIONSTATS_H_
