#ifndef SRC_DEBUG_CPUPROFILER_H_
#define SRC_DEBUG_CPUPROFILER_H_

#include <array>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

namespace pwg::debug
{
	/**
	* @brief One CPU scope of a frame. Times are steady_clock ticks; parent is -1 for top-level scopes.
	*/
	struct CpuScopeRecord
	{
		const char* name{ nullptr };
		std::chrono::steady_clock::time_point start{};
		std::chrono::steady_clock::time_point end{};
		int16_t parent{ -1 };
		uint16_t depth{ 0 };
		bool closed{ false };
	};

	/**
	* @brief A timed piece of work reported by any thread (for example the hydrology worker).
	* frameIndex is the main-thread frame in which the event was drained.
	*/
	struct CpuEvent
	{
		const char* name{ nullptr };
		double ms{ 0.0 };
		uint64_t frameIndex{ 0 };
	};

	/**
	* @brief CPU scopes on std::chrono::steady_clock.
	* Scopes are main-thread only and are kept per frame in a preallocated array. Other threads report
	* finished work through the mutex-protected event sink, which the main thread drains once per frame.
	*/
	class CpuProfiler
	{
	public:
		using Clock = std::chrono::steady_clock;

		static constexpr uint32_t MaxScopesPerFrame = 256;
		static constexpr uint32_t RecentEventCount = 64;

		CpuProfiler();

		/** @brief Clears the scope array of the new frame. */
		void BeginFrame(uint64_t frameIndex);

		/** @brief Publishes the finished frame's scopes and drains the event sink. */
		void EndFrame();

		/**
		* @brief Opens a scope. Returns its index, or -1 if the scope is not recorded (wrong thread, array full).
		*/
		int32_t BeginScope(const char* name);

		/** @brief Closes a scope opened in the same frame. */
		void EndScope(int32_t index, uint64_t frameIndex);

		/** @brief Thread-safe. Reports a finished piece of work measured by the caller with steady_clock. */
		void SubmitEvent(const char* name, Clock::time_point start, Clock::time_point end);

		uint64_t GetFrameIndex() const { return m_frameIndex; }

		/** @brief Scopes of the last finished frame, in opening order. */
		const std::vector<CpuScopeRecord>& GetLastFrameScopes() const { return m_lastFrame; }

		/** @brief Most recent drained events, oldest first. */
		std::vector<CpuEvent> GetRecentEvents() const;

		/** @brief Scopes that did not fit into the per-frame array since start. */
		uint64_t GetOverflowCount() const { return m_overflow; }

	private:
		std::array<CpuScopeRecord, MaxScopesPerFrame> m_current{};
		uint32_t m_count{ 0 };
		int32_t m_open{ -1 };
		uint64_t m_frameIndex{ 0 };
		uint64_t m_overflow{ 0 };
		std::thread::id m_mainThread;
		std::vector<CpuScopeRecord> m_lastFrame;

		std::mutex m_eventMutex;
		std::vector<CpuEvent> m_pendingEvents;							/**< Guarded by m_eventMutex. */
		std::array<CpuEvent, RecentEventCount> m_recentEvents{};
		uint32_t m_recentHead{ 0 };
		uint32_t m_recentCount{ 0 };
	};
} // namespace pwg::debug

#endif // !SRC_DEBUG_CPUPROFILER_H_
