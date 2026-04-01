#ifndef SRC_TERRAIN_GRAPH_PASSES_ELEVATION_PASS_H_
#define SRC_TERRAIN_GRAPH_PASSES_ELEVATION_PASS_H_

#include "terrainPass.h"

namespace pwg
{
	struct ElevationPassParameters
	{
		int noiseType = 0;
		int seed = 1;
		float amplitude = 0.01f;
		float frequency = 0.5f;
		float scale = 100.0f;
		glm::vec2 offset{ 0.0f, 0.0f };
	};

	class ElevationPass : public TerrainPass
	{
	public:
		ElevationPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize);
		~ElevationPass() = default;

		void Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx) override;
		std::vector<UIParameters> GetParameters() override;
		std::string GetName() override;

	private:
		std::string m_name = "ElevationPass";
		TerrainPassTypes m_passType = TerrainPassTypes::GENERATION;
		std::shared_ptr<ComputeShader> m_computeShader;
		std::vector<std::string> m_inputTextures;
		std::vector<std::string> m_outputTextures;
		ElevationPassParameters m_parameters;
		int m_terrainSize = 0;
	};
}
#endif // !SRC_TERRAIN_GRAPH_PASSES_ELEVATION_PASS_H_
