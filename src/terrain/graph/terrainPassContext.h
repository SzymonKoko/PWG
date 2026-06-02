#ifndef SRC_TERRAIN_GRAPH_TERRAIN_PASS_CONTEXT_H_
#define SRC_TERRAIN_GRAPH_TERRAIN_PASS_CONTEXT_H_

#include <glm/vec2.hpp>
#include <vector>

namespace pwg
{
	struct HydraulicSimulationSettings
	{
		int stepsPerUpdate = 1;
		int waterStepsPerSecond = 6;
		int waterSubsteps = 1;
		int bakeParticleSteps = 24;
		int bakeStepsPerFrame = 1;
		int bakeUploadInterval = 4;
		bool erosionEnabled = true;
		bool autoWaterSimulation = false;
		bool particleErosionEnabled = true;
		bool showWaterSurface = true;
		bool waterRefractionEnabled = false;
		bool useDiagonalFlow = false;

		float rainRate = 0.0025f;
		float rainElevationBias = 1.0f;
		float rainMinHeight = 0.55f;
		float rainMaxHeight = 0.95f;

		float flowRate = 0.55f;
		float flowInertia = 0.0f;
		float boundaryDrain = 0.85f;
		float lakeLeveling = 0.22f;
		float maxWaterDepth = 1.4f;
		float maxSurfaceDrop = 0.22f;

		float evaporation = 0.0006f;
		float infiltrationRate = 0.0004f;
		float lakeInfiltrationScale = 0.05f;

		float sedimentCapacity = 16.0f;
		float erosionRate = 0.16f;
		float slopeErosionRate = 0.42f;
		float slopeErosionMinSlope = 0.006f;
		float slopeErosionWaterScale = 1.25f;
		float depositionRate = 0.10f;
		float valleyDepositionRate = 0.28f;
		float valleyDepositionSlope = 0.09f;

		float steepSlopeDepositionReduction = 0.9f;
		float depositionSlopeStart = 0.04f;
		float depositionSlopeEnd = 0.22f;

		float minWaterToErode = 0.001f;
		float minFlowToErode = 0.0005f;

		float maxHeightChange = 0.032f;
		float maxErosionDepth = 1.2f;
		float maxDepositionHeight = 0.80f;

		float lakeDeposition = 0.65f;
		float lakeMinDepth = 0.035f;
		float lakeBedSmoothing = 0.04f;
		float lakeWaterSmoothing = 0.22f;

		float channelSmoothing = 0.03f;

		float flowAccumulationGrowth = 0.85f;
		float flowAccumulationDecay = 0.015f;

		float riverChannelGrowth = 0.14f;
		float riverChannelDecay = 0.001f;
		float riverChannelInfluence = 0.35f;
		float riverChannelDepth = 0.09f;

		int particlesPerStep = 2500;
		int dropletLifetime = 36;
		int particleSeed = 1337;
		float particleInertia = 0.22f;
		float particleGravity = 5.5f;
		float particleInitialWater = 1.0f;
		float particleEvaporation = 0.018f;
		float particleMinSlope = 0.0008f;
		float particleCapacity = 5.0f;
		float particleErosionStrength = 0.045f;
		float particleDepositionStrength = 0.18f;
		float particleBrushRadius = 2.0f;
		float particleDepositRadius = 5.0f;
		float particleDepositSlopeStart = 0.012f;
		float particleDepositSlopeEnd = 0.085f;
		float particleDepositValleyBoost = 2.0f;
		float particleDepositShelfBoost = 0.65f;
		float particleDepositMinSuitability = 0.08f;
		float looseSedimentErosionStrength = 1.0f;
		float looseSedimentSmoothing = 0.12f;
		float particleChannelStrength = 0.04f;
		float particleFlowStrength = 0.015f;
		float lakeFillEnabled = 1.0f;
		float lakeMinFlow = 0.015f;
		float lakeMaxDepth = 2.5f;
	};

	struct TerrainPassContext
	{
		int chunkSize;
		glm::vec2 offset{ 0.0f, 0.0f };
		int simulationIterations = 40;
		HydraulicSimulationSettings hydraulicSettings;
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
