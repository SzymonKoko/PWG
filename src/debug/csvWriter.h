#ifndef SRC_DEBUG_CSVWRITER_H_
#define SRC_DEBUG_CSVWRITER_H_

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include "debug/frameStats.h"
#include "debug/runMetadata.h"
#include "debug/simulationStats.h"

namespace pwg::debug
{
	/**
	* @brief One row of events.csv: one measured value of one event (design section 7.3, long format).
	*
	* D-03 events become rows through AppendSimEventRows. Later producers (memory snapshots of D-04, the
	* stepping window of D-11) add rows of their own event names through the same struct, so the file
	* format does not change when they are added.
	*/
	struct EventRow
	{
		uint64_t engineFrame{ 0 };				/**< Frame index of the engine at which the event started. */
		std::string event;						/**< Event name, e.g. hydraulic_step. */
		uint64_t sequence{ 0 };					/**< Event sequence number; rows of one event share it. */
		uint64_t group{ 0 };					/**< Graph run or hydrology run of the event, 0 = none (empty). */
		int64_t index{ -1 };					/**< Step index, node order or stage number, -1 = none (empty). */
		std::string detail;						/**< `key=value` pairs separated by `;`. */
		std::string metric;						/**< cpu_ms, gpu_ms, active_particles, ... */
		double value{ NotAvailable };			/**< NaN = not available (empty field). */
		const char* unit{ "" };					/**< ms, 1, frame, B */
		const char* source{ "" };				/**< Clock or timer that produced the value, e.g. steady_clock, time_elapsed. */
	};

	/**
	* @brief Converts a complete D-03 event into events.csv rows. `order` is the index used for graph nodes
	* (execution order within the run); other events take their step index or stage number.
	*/
	void AppendSimEventRows(const SimEvent& event, int64_t order, std::vector<EventRow>& out);

	/** @brief Outcome of one export, shown in the Performance tab and returned to the runner. */
	struct ExportResult
	{
		bool written{ false };
		std::filesystem::path folder;
		uint64_t frames{ 0 };
		uint64_t eventRows{ 0 };
		uint64_t summaryRows{ 0 };
		double writeMs{ NotAvailable };
		uint64_t gpuDropped{ 0 };				/**< Sum of gpu_dropped over the window (0 when instrumentation is compiled out). */
		double maxSleepMs{ 0.0 };				/**< Largest cpu_sleep_ms of the window; the runner checks it in benchmark mode. */
		std::vector<std::string> warnings;
		std::string error;
	};

	/**
	* @brief Records one measurement window in memory and writes it to frames.csv, events.csv, summary.csv
	* and run.yaml (design sections 7.1 to 7.3).
	*
	* The window starts with the frame after Start and ends with the frame in which Stop is called, or
	* after a fixed frame count. Inside the window nothing is written to disk: each frame record is copied
	* from the FrameStats ring once its GPU results were harvested or declared dropped (K frames after the
	* frame), and complete D-03 events are copied by SimulationStats into the recorder's capture list.
	* After the last frame the recorder keeps going ("draining") until every frame of the window was copied
	* and no GPU event that started inside the window is pending, or until a timeout. Only then are the
	* files written, at the start of a frame outside the window.
	*
	* Main thread only. When idle it costs one comparison per frame. It runs in every build: with the
	* instrumentation compiled out the files hold the frame timing, and the GPU and counter fields are empty.
	*/
	class CsvRecorder
	{
	public:
		using Clock = std::chrono::steady_clock;

		enum class State : uint8_t
		{
			IDLE,
			RECORDING,
			DRAINING
		};

		enum class StopReason : uint8_t
		{
			NONE,
			FRAME_COUNT,	/**< The configured number of frames was reached. */
			USER,			/**< Stop was called. */
			FRAME_LIMIT		/**< MaxFrames was reached. */
		};

		static constexpr uint64_t MaxFrames = 1'000'000;				/**< Frames per window, bounds the memory use. */
		static constexpr uint64_t MaxEvents = 1'000'000;				/**< Captured events per window. */
		static constexpr uint64_t DefaultDrainTimeoutFrames = 600;		/**< Frames to wait for pending GPU events. */

		CsvRecorder(const FrameStats& frameStats, SimulationStats& simulationStats);
		~CsvRecorder();
		CsvRecorder(const CsvRecorder&) = delete;
		CsvRecorder& operator=(const CsvRecorder&) = delete;

		/**
		* @brief Starts a window that begins with frame `currentFrame + 1`. `frameCount` 0 records until Stop.
		* The files go to `folder`; it may exist (the runner puts masks and screenshots there first) but must not
		* hold frames.csv, events.csv, summary.csv or run.yaml. Returns false if a window is already active.
		*/
		bool Start(const RunInfo& run, Metadata metadata, std::filesystem::path folder, uint64_t currentFrame, uint64_t frameCount);

		/** @brief Ends the window with frame `currentFrame` (the frame in progress). */
		void Stop(uint64_t currentFrame);

		/** @brief Drops the window without writing anything. */
		void Abort();

		/**
		* @brief Adds a row of another producer (the runner's `stepping_window`) to events.csv. Only while a window
		* records, and only for a frame inside it; returns false otherwise.
		*/
		bool AddEventRow(EventRow row);

		/**
		* @brief Frame hook, called by DebugView at the end of BeginFrame of `frameIndex`, after the GPU
		* results of frame `frameIndex - lagFrames` were harvested. Copies finished frames, and writes the
		* files once the window has drained.
		*/
		void OnFrameBegin(uint64_t frameIndex, uint32_t lagFrames)
		{
			if (m_state != State::IDLE)
			{
				Advance(frameIndex, lagFrames);
			}
		}

		State GetState() const { return m_state; }
		bool IsActive() const { return m_state != State::IDLE; }
		uint64_t GetRecordedFrames() const { return m_frames.size(); }
		uint64_t GetCapturedEvents() const { return m_events.size(); }
		uint64_t GetFrameCount() const { return m_frameCount; }
		const std::filesystem::path& GetFolder() const { return m_folder; }
		const ExportResult& GetLastResult() const { return m_lastResult; }
		bool HasResult() const { return m_hasResult; }

	private:
		void Advance(uint64_t frameIndex, uint32_t lagFrames);
		void Finish(uint64_t frameIndex);
		ExportResult Write(uint64_t frameIndex);
		void Reset();

		const FrameStats& m_frameStats;
		SimulationStats& m_sim;

		State m_state{ State::IDLE };
		StopReason m_stopReason{ StopReason::NONE };
		RunInfo m_run;
		Metadata m_metadata;
		std::filesystem::path m_folder;
		uint64_t m_firstFrame{ 0 };
		uint64_t m_lastFrame{ UINT64_MAX };		/**< Inclusive; UINT64_MAX until known. */
		uint64_t m_frameCount{ 0 };
		uint64_t m_nextFrame{ 0 };				/**< Next frame to copy from the ring. */
		uint64_t m_missingFrames{ 0 };			/**< Frames no longer in the ring when copied (should stay 0). */
		uint64_t m_drainStartFrame{ 0 };
		bool m_eventLimitReached{ false };
		std::vector<FrameRecord> m_frames;
		std::vector<SimEvent> m_events;
		std::vector<EventRow> m_extraRows;		/**< Rows added through AddEventRow. */

		ExportResult m_lastResult;
		bool m_hasResult{ false };
	};
} // namespace pwg::debug

#endif // !SRC_DEBUG_CSVWRITER_H_
