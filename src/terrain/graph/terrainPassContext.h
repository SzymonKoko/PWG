#ifndef SRC_TERRAIN_GRAPH_TERRAIN_PASS_CONTEXT_H_
#define SRC_TERRAIN_GRAPH_TERRAIN_PASS_CONTEXT_H_

#include <glm/vec2.hpp>
#include <vector>

namespace pwg
{
	struct HydraulicSimulationSettings
	{
		bool showWaterSurface = true;
		bool showErosionParticles = false;
		float particleSimulationSpeed = 1.0f;
		bool waterRefractionEnabled = true;
		float minVisibleWaterDepth = 0.025f;

		// Deterministic drainage-network generator.
		float riverAmount = 0.5f;
		float lakeAmount = 0.5f;
		int hydrologySeed = 1337;

	};

	struct ThermalSimulationSettings
	{
		float simulationSpeed = 1.0f;
	};

	struct TerrainPassContext
	{
		int chunkSize;
		glm::vec2 offset{ 0.0f, 0.0f };
		glm::vec2 worldOrigin{ 0.0f, 0.0f };
		int worldSize{ 0 };
		bool chunkLocalGeneration{ false };
		int simulationIterations = 40;
		HydraulicSimulationSettings hydraulicSettings;
		ThermalSimulationSettings thermalSettings;
		bool cpuTerrainCacheValid = false;
		std::vector<float> cpuHeight;
		std::vector<float> cpuBaseHeight;
		std::vector<float> cpuDeposit;
		std::vector<float> cpuWater;
		std::vector<float> cpuSediment;
		std::vector<float> cpuFlowAccumulation;
		std::vector<float> cpuRiverChannels;
		std::vector<float> cpuErosionDelta;
		bool deferParticleTextureUpload = false;

	};
}
#endif // !SRC_TERRAIN_GRAPH_TERRAIN_PASS_CONTEXT_H_
