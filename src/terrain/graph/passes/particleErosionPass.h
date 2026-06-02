#ifndef SRC_TERRAIN_GRAPH_PASSES_PARTICLE_EROSION_PASS_H_
#define SRC_TERRAIN_GRAPH_PASSES_PARTICLE_EROSION_PASS_H_

#include "terrainPass.h"
#include <random>

namespace pwg
{
	struct ParticleErosionPassParameters
	{
		HydraulicSimulationSettings* settings = nullptr;
	};

	class ParticleErosionPass : public TerrainPass
	{
	public:
		ParticleErosionPass(int terrainSize);
		~ParticleErosionPass() = default;

		void SetHydraulicSettings(HydraulicSimulationSettings* settings);
		void Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx) override;
		std::vector<UIParameters> GetParameters() override;
		TerrainPassTypes GetPassType() override;
		std::string GetName() override;

	private:
		struct HeightSample
		{
			float height{ 0.0f };
			glm::vec2 gradient{ 0.0f };
		};

		void EnsureCpuCache(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx);
		void UploadCpuCache(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx);
		void SimulateDroplet(TerrainPassContext& ctx, std::mt19937& rng, const std::vector<float>& heightSnapshot, const std::vector<float>& depositSnapshot, std::vector<float>& deltaHeight, std::vector<float>& deltaDeposit, std::vector<float>& deltaFlow, std::vector<float>& deltaRiver, std::vector<float>& deltaErosion);
		float DepositAt(const TerrainPassContext& ctx, const std::vector<float>& heightSnapshot, float x, float y, float amount, std::vector<float>& deltaHeight, std::vector<float>& deltaDeposit, std::vector<float>& deltaErosion);
		float ErodeAt(TerrainPassContext& ctx, float x, float y, float amount, const std::vector<float>& heightSnapshot, const std::vector<float>& depositSnapshot, std::vector<float>& deltaHeight, std::vector<float>& deltaDeposit, std::vector<float>& deltaErosion);
		void AddFlowAt(float x, float y, float amount, std::vector<float>& deltaFlow, std::vector<float>& deltaRiver, const TerrainPassContext& ctx);
		void FillLakes(TerrainPassContext& ctx);
		void SmoothLooseSediment(TerrainPassContext& ctx);
		float DepositionSuitability(const TerrainPassContext& ctx, const std::vector<float>& heightSnapshot, int x, int y) const;
		HeightSample SampleHeight(const std::vector<float>& height, float x, float y) const;
		float SampleValue(const std::vector<float>& values, float x, float y) const;
		int Index(int x, int y) const;

		std::string m_name = "2. Particle hydraulic erosion";
		TerrainPassTypes m_passType = TerrainPassTypes::EROSION;
		ParticleErosionPassParameters m_parameters;
		int m_terrainSize{ 0 };
		int m_stepCounter{ 0 };
	};
}

#endif
