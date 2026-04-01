#ifndef SRC_TERRAIN_GRAPH_PASSES_SLOPE_MASK_PASS_H_
#define SRC_TERRAIN_GRAPH_PASSES_SLOPE_MASK_PASS_H_

#include "terrainPass.h"

namespace pwg
{
	class SlopeMaskPass : public TerrainPass
	{
	public:
		SlopeMaskPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize);
		~SlopeMaskPass() = default;

		void Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx) override;
		std::vector<UIParameters> GetParameters() override;
		std::string GetName() override;

	private:
		std::string m_name = "SlopeMaskPass";
		TerrainPassTypes m_passType = TerrainPassTypes::MASK;
		std::shared_ptr<ComputeShader> m_computeShader;
		std::vector<std::string> m_inputTextures;
		std::vector<std::string> m_outputTextures;
		int m_terrainSize = 0;
	};
}
#endif // !SRC_TERRAIN_GRAPH_PASSES_SLOPE_MASK_PASS_H_
