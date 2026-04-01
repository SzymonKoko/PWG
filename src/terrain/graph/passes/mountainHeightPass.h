#ifndef SRC_TERRAIN_GRAPH_PASSES_MOUNTAIN_HEIGHT_PASS_H_
#define SRC_TERRAIN_GRAPH_PASSES_MOUNTAIN_HEIGHT_PASS_H_

#include "terrainPass.h"

namespace pwg
{
	struct MountainHeightPassParameters
	{
		int noiseType{ 0 };
		float frequency{ 0.01 };
		float amplitude{ 0.01 };
		float scale{ 100.0 };
		glm::vec2 offset{ 0.0, 0.0 };
		int seed{ 1 };

		int fractalType{ 0 };
		int octaves{ 1 };
		float lacunarity{ 2.0 };
		float persistance{ 0.5 };

		float ridgeStrength{ 0.0 };

		bool falloff{ true };
		float falloffStrength{ 1.0 };

	};

	class MountainHeightPass : public TerrainPass
	{
	public:
		MountainHeightPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize);
		~MountainHeightPass() = default;

		void Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx) override;
		std::vector<UIParameters> GetParameters() override;
		std::string GetName() override;

	private:
		std::string m_name = "MountainHeightPass";
		TerrainPassTypes m_passType = TerrainPassTypes::GENERATION;
		std::shared_ptr<ComputeShader> m_computeShader;
		std::vector<std::string> m_inputTextures;
		std::vector<std::string> m_outputTextures;
		MountainHeightPassParameters m_parameters;
		int m_terrainSize = 0;
	};
}
#endif // !SRC_TERRAIN_GRAPH_PASSES_MOUNTAIN_HEIGHT_PASS_H_
