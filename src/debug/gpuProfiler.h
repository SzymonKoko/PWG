#ifndef SRC_DEBUG_GPUPROFILER_H_
#define SRC_DEBUG_GPUPROFILER_H_

#include <array>
#include <cstdint>
#include <vector>
#include "debug/frameStats.h"

namespace pwg::debug
{
	/**
	* @brief Non-blocking GPU timing with query objects (OpenGL 4.5 spec 4.2, 4.3).
	*
	* Each pass has one TIME_ELAPSED query per frame slot in a ring of K slots. A TIMESTAMP pair spans the
	* scene and ImGui rendering of the frame. At the start of frame n the slot n mod K, which holds frame
	* n - K, is harvested: a result is read with GL_QUERY_RESULT only after GL_QUERY_RESULT_AVAILABLE
	* returned GL_TRUE. A query that is not ready is dropped and counted, never waited for and never
	* written as 0. At most one TIME_ELAPSED query is active at a time, and each pass is timed at most once
	* per frame. All queries live in the main GL context (query objects are not shared between contexts).
	*/
	class GpuProfiler
	{
	public:
		static constexpr uint32_t DefaultRingFrames = 4;
		static constexpr uint32_t MaxRingFrames = 16;

		GpuProfiler() { m_latestPassMs.fill(NotAvailable); }
		~GpuProfiler() = default;
		GpuProfiler(const GpuProfiler&) = delete;
		GpuProfiler& operator=(const GpuProfiler&) = delete;

		/**
		* @brief Creates the query objects in the current (main) GL context. Calling it again recreates the
		* ring with a new size: results of the old ring that are available are written to `stats`, the
		* others are counted as dropped.
		*/
		void Init(uint32_t ringFrames, FrameStats* stats = nullptr);

		/** @brief Deletes the query objects. Must run while the main GL context is current. */
		void Shutdown();

		/** @brief Harvests the slot of frame n - K into `stats` and assigns the slot to `frameIndex`. */
		void BeginFrame(uint64_t frameIndex, FrameStats& stats);

		/** @brief Closes a pass left active by mistake, so the next frame can begin a TIME_ELAPSED query. */
		void EndFrame();

		/** @brief Starts the TIME_ELAPSED query of a pass. Returns false if the pass is not timed. */
		bool BeginPass(GpuPass pass);
		void EndPass(GpuPass pass);

		/** @brief First and second TIMESTAMP of the gpu_frame_ms span. */
		void MarkFrameStart();
		void MarkFrameEnd();

		uint32_t GetRingFrames() const { return static_cast<uint32_t>(m_slots.size()); }
		uint64_t GetDroppedCount() const { return m_dropped; }
		uint64_t GetHarvestedCount() const { return m_harvested; }
		int GetTimeElapsedBits() const { return m_timeElapsedBits; }
		int GetTimestampBits() const { return m_timestampBits; }

		/** @brief Latest available GPU time of a pass (ms), NotAvailable before the first result. */
		double GetLatestPassMs(GpuPass pass) const { return m_latestPassMs[static_cast<size_t>(pass)]; }
		double GetLatestFrameMs() const { return m_latestFrameMs; }

	private:
		struct Slot
		{
			uint64_t frameIndex{ 0 };
			std::array<unsigned int, GpuPassCount> passQueries{};
			std::array<bool, GpuPassCount> passIssued{};
			unsigned int frameStartQuery{ 0 };
			unsigned int frameEndQuery{ 0 };
			bool frameStartIssued{ false };
			bool frameEndIssued{ false };
		};

		/** @brief Reads every issued query of the slot that is available and drops the rest. Never blocks. */
		void HarvestSlot(Slot& slot, FrameStats* stats);
		bool ReadIfAvailable(unsigned int query, uint64_t& result);
		void DeleteQueries();

		std::vector<Slot> m_slots;
		Slot* m_current{ nullptr };
		GpuPass m_activePass{ GpuPass::COUNT };
		uint64_t m_dropped{ 0 };
		uint64_t m_harvested{ 0 };
		int m_timeElapsedBits{ 0 };
		int m_timestampBits{ 0 };
		bool m_reportedMisuse{ false };
		std::array<double, GpuPassCount> m_latestPassMs{};
		double m_latestFrameMs{ NotAvailable };
	};
} // namespace pwg::debug

#endif // !SRC_DEBUG_GPUPROFILER_H_
