#ifndef SRC_TERRAIN_GRAPH_PASSES_MOUNTAIN_MASK_PASS_H_
#define SRC_TERRAIN_GRAPH_PASSES_MOUNTAIN_MASK_PASS_H_

#include "terrainPass.h"

namespace pwg
{
	struct MountainMaskPassParameters
	{
		float minThreshold = 0.4f;
		float maxThreshold = 0.7f;
	};

	class MountainMaskPass : public TerrainPass
	{
	public:
		MountainMaskPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize);
		~MountainMaskPass() = default;

		void Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx) override;
		std::vector<UIParameters> GetParameters() override;
		std::string GetName() override;

	private:
		std::string m_name = "MountainMaskPass";
		TerrainPassTypes m_passType = TerrainPassTypes::MASK;
		std::shared_ptr<ComputeShader> m_computeShader;
		std::vector<std::string> m_inputTextures;
		std::vector<std::string> m_outputTextures;
		MountainMaskPassParameters m_parameters;
		int m_terrainSize = 0;
	};
}
#endif // !SRC_TERRAIN_GRAPH_PASSES_MOUNTAIN_MASK_PASS_H_
