#ifndef SRC_TERRAIN_HYDROLOGY_DRAINAGE_NETWORK_GENERATOR_H_
#define SRC_TERRAIN_HYDROLOGY_DRAINAGE_NETWORK_GENERATOR_H_

#include <atomic>
#include <future>
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
