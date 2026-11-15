#ifndef SRC_DEBUG_FRAMESTATS_H_
#define SRC_DEBUG_FRAMESTATS_H_

#include <array>
#include <chrono>
#include <cstdint>
#include <limits>
#include <vector>

namespace pwg::debug
{
	/**
	* @brief GPU passes timed with TIME_ELAPSED queries. They run one after another and never overlap.
	*/
	enum class GpuPass : uint8_t
	{
		SHADOW,
		TERRAIN,
		VEGETATION,
		WATER,
		POST_PROCESS,
		IMGUI,
		COUNT
	};

	constexpr size_t GpuPassCount = static_cast<size_t>(GpuPass::COUNT);

	/**
	* @brief Short pass name used in the UI (and later as the CSV column suffix).
	*/
	const char* GetGpuPassName(GpuPass pass);

	/**
	* @brief Frame-level CPU phases measured with steady_clock in every build.
	*/
	enum class FramePhase : uint8_t
	{
		UPDATE,		/**< Application::Update (input, Gui::Update, scene update and scene draw submission). */
		RENDER,		/**< Application::Render without SwapBuffers. */
		SWAP,		/**< Window::SwapBuffers. */
		SLEEP,		/**< Frame limiter sleep in Application::Run. */
		COUNT
	};

	constexpr size_t FramePhaseCount = static_cast<size_t>(FramePhase::COUNT);

	/** @brief Marker for "value not available". Never replaced by 0. */
	constexpr double NotAvailable = std::numeric_limits<double>::quiet_NaN();

	/**
	* @brief Per-frame measurements. GPU values arrive some frames later and are filled in by frame index.
	*/
	struct FrameRecord
	{
		uint64_t frameIndex{ 0 };
		double frameMs{ NotAvailable };										/**< Time between this frame's begin and the next frame's begin. */
		std::array<double, FramePhaseCount> phaseMs{};						/**< CPU time per FramePhase. */
		double gpuFrameMs{ NotAvailable };									/**< TIMESTAMP span over scene and ImGui rendering. */
		std::array<double, GpuPassCount> gpuPassMs{};						/**< TIME_ELAPSED per pass, NotAvailable if not timed. */
		uint32_t gpuDropped{ 0 };											/**< GPU samples of this frame that were not ready when their slot was reused. */
	};

	/**
	* @brief Summary of one metric over the rolling window.
	* Percentiles use the nearest-rank definition: rank = ceil(p / 100 * n) on the sorted sample.
	*/
	struct RollingStats
	{
		uint32_t count{ 0 };
		double mean{ NotAvailable };
		double median{ NotAvailable };
		double p1{ NotAvailable };
		double p99{ NotAvailable };
		double min{ NotAvailable };
		double max{ NotAvailable };
	};

	/**
	* @brief Fixed-size ring of frame records with rolling statistics for the UI.
	*/
	class FrameStats
	{
	public:
		using Clock = std::chrono::steady_clock;

		static constexpr uint32_t DefaultCapacity = 2048;		/**< Frames kept in the ring. */
		static constexpr uint32_t DefaultWindow = 600;			/**< Frames in the rolling statistics. */

		explicit FrameStats(uint32_t capacity = DefaultCapacity, uint32_t window = DefaultWindow);

		/**
		* @brief Starts a new record for the given frame index, overwriting the oldest one.
		*/
		FrameRecord& BeginRecord(uint64_t frameIndex);

		/**
		* @brief Returns the record of a frame if it is still in the ring, otherwise nullptr.
		*/
		FrameRecord* Find(uint64_t frameIndex);
		const FrameRecord* Find(uint64_t frameIndex) const;

		/**
		* @brief Marks a frame as complete (its frameMs is known). Rolling statistics use complete frames only.
		*/
		void Complete(uint64_t frameIndex) { m_lastComplete = frameIndex; m_hasComplete = true; }

		/**
		* @brief Recomputes the rolling statistics if at least 0.5 s passed since the last update.
		*/
		void UpdateRolling(Clock::time_point now, bool force = false);

		const RollingStats& GetFrameMsStats() const { return m_frameMsStats; }
		const RollingStats& GetGpuFrameMsStats() const { return m_gpuFrameMsStats; }
		uint32_t GetWindow() const { return m_window; }

		/**
		* @brief Copies frameMs of up to `count` most recent complete frames, oldest first.
		*/
		void CopyRecentFrameMs(std::vector<float>& out, uint32_t count) const;

		/**
		* @brief Summarises a sample (mean, nearest-rank median/p1/p99, min, max). Reorders the input.
		*/
		static RollingStats Compute(std::vector<double>& samples);

	private:
		std::vector<FrameRecord> m_records;
		uint32_t m_window;
		uint64_t m_lastComplete{ 0 };
		bool m_hasComplete{ false };
		Clock::time_point m_lastRollingUpdate{};
		RollingStats m_frameMsStats;
		RollingStats m_gpuFrameMsStats;
		std::vector<double> m_scratch;
	};
} // namespace pwg::debug

#endif // !SRC_DEBUG_FRAMESTATS_H_
