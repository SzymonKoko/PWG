#ifndef SRC_TERRAIN_HYDROLOGY_DRAINAGE_NETWORK_GENERATOR_H_
#define SRC_TERRAIN_HYDROLOGY_DRAINAGE_NETWORK_GENERATOR_H_

#include <array>
#include <atomic>
#include <future>
#include <limits>
#include <string>
#include <vector>

namespace pwg
{
	struct DrainageNetworkSettings
	{
		float riverAmount{ 0.5f };
		float lakeAmount{ 0.5f };
		int seed{ 1337 };
	};

	struct DrainageNetworkResult
	{
		int size{ 0 };
		bool cancelled{ false };
		std::vector<float> baseHeight;
		std::vector<float> finalHeight;
		std::vector<float> water;
		std::vector<float> flow;
		std::vector<float> rivers;
		std::vector<float> lakes;
		std::vector<float> velocity;
		std::vector<float> carveDelta;

		// Worker wall time (steady_clock, ms) for the Debug View. Written only by the worker thread that
		// builds the result and read by the main thread after the future is ready. NaN if not measured.
		static constexpr size_t StageCount = 6;		/**< Stages 1..6 do work; stage 7 means "ready". */
		static constexpr double NotMeasured = std::numeric_limits<double>::quiet_NaN();
		std::array<double, StageCount> stageMs{ NotMeasured, NotMeasured, NotMeasured, NotMeasured, NotMeasured, NotMeasured };	/**< Index = stage - 1. */
		double resampleMs{ NotMeasured };		/**< Resampling at the start of stage 1. */
		double workerMs{ NotMeasured };			/**< Whole Generate call up to the last finished stage. */

		bool IsValid() const;
	};

	class DrainageNetworkGenerator
	{
	public:
		~DrainageNetworkGenerator();

		bool Start(std::vector<float> height, int size, DrainageNetworkSettings settings);
		bool TryTakeResult(DrainageNetworkResult& result);
		void Cancel();
		bool IsRunning() const;
		float GetProgress() const;
		std::string GetStage() const;
		static const char* GetStageName(int stage);

	private:
		static DrainageNetworkResult Generate(std::vector<float> height, int size,
			DrainageNetworkSettings settings, std::atomic<bool>& cancel,
			std::atomic<float>& progress, std::atomic<int>& stage);

		std::future<DrainageNetworkResult> m_future;
		std::atomic<bool> m_cancel{ false };
		std::atomic<bool> m_running{ false };
		std::atomic<float> m_progress{ 0.0f };
		std::atomic<int> m_stage{ 0 };
	};
}

#endif
