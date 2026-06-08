#ifndef SRC_TERRAIN_GRAPH_TERRAIN_PASS_CONTEXT_H_
#define SRC_TERRAIN_GRAPH_TERRAIN_PASS_CONTEXT_H_

#include <glm/vec2.hpp>
#include <vector>

namespace pwg
{
	struct HydraulicSimulationSettings
	{
		int stepsPerUpdate = 1;
		int waterStepsPerSecond = 8;
		int waterSubsteps = 2;
		int bakeParticleSteps = 24;
		int bakeStepsPerFrame = 1;
		int bakeUploadInterval = 4;
		bool erosionEnabled = true;
		bool autoWaterSimulation = false;
		bool particleErosionEnabled = false;
		bool showWaterSurface = true;
		bool waterRefractionEnabled = false;
		bool useDiagonalFlow = true;

		// Water budget
		float rainRate = 0.004f;
		float rainElevationBias = 0.35f;
		float rainMinHeight = 0.0f;
		float rainMaxHeight = 1.2f;
		float evaporation = 0.00004f;
		float infiltrationRate = 0.00003f;
		float boundaryDrain = 0.92f;
		float maxWaterDepth = 2.5f;

		// Virtual pipes
		float pipeFlow = 0.38f;
		float pipeDamping = 0.28f;
		float flowConcentration = 0.40f;

		// Lakes
		float lakeMinDepth = 0.012f;
		float lakeLeveling = 0.48f;

		// Erosion (stream-power)
		float sedimentCapacity = 4.0f;
		float erosionStrength = 0.20f;
		float depositionStrength = 0.014f;
		float flatteningStrength = 0.25f;
		float minWaterToErode = 0.0008f;
		float minFlowToErode = 0.0003f;
		float maxHeightChange = 0.018f;
		float maxErosionDepth = 1.0f;
		float maxDepositHeight = 0.06f;

		// River channel feedback
		float flowAccumulationGrowth = 0.90f;
		float flowAccumulationDecay = 0.008f;
		float riverChannelGrowth = 0.22f;
		float riverChannelDecay = 0.004f;
		float riverChannelInfluence = 0.55f;
		float riverChannelDepth = 0.14f;

		// Legacy fields kept for project file compatibility
		float flowRate = 0.38f;
		float flowInertia = 0.28f;
		float maxSurfaceDrop = 0.22f;
		float lakeInfiltrationScale = 0.03f;
		float lakeWaterSmoothing = 0.30f;
		float erosionRate = 0.14f;
		float slopeErosionRate = 0.35f;
		float slopeErosionMinSlope = 0.006f;
		float slopeErosionWaterScale = 1.25f;
		float depositionRate = 0.022f;
		float valleyDepositionRate = 0.22f;
		float valleyDepositionSlope = 0.09f;
		float depositDiffusion = 0.0f;
		float steepSlopeDepositionReduction = 0.9f;
		float depositionSlopeStart = 0.04f;
		float depositionSlopeEnd = 0.22f;
		float maxDepositionHeight = 0.10f;
		float lakeDeposition = 0.72f;
		float lakeBedSmoothing = 0.06f;
		float channelSmoothing = 0.06f;
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
		glm::vec2 worldOrigin{ 0.0f, 0.0f };
		int worldSize{ 0 };
		bool chunkLocalGeneration{ false };
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
