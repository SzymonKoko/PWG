#ifndef SRC_TERRAIN_GRAPH_PASSES_FINAL_MASK_PASS_H_
#define SRC_TERRAIN_GRAPH_PASSES_FINAL_MASK_PASS_H_

#include "terrainPass.h"

namespace pwg
{
	class FinalMaskPass : public TerrainPass
	{
	public:
		FinalMaskPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize);
		~FinalMaskPass() = default;

		void Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx) override;
		std::vector<UIParameters> GetParameters() override;
		std::string GetName() override;

	private:
		std::string m_name = "FinalMaskPass";
		TerrainPassTypes m_passType = TerrainPassTypes::GENERATION;
		std::shared_ptr<ComputeShader> m_computeShader;
		std::vector<std::string> m_inputTextures;
		std::vector<std::string> m_outputTextures;
		std::vector<std::shared_ptr<Texture>> m_inputHeightTextures;
		std::vector<float> m_inputHeightTexturesWeights;

		int m_terrainSize{ 0 };
	};
}
#endif // !SRC_TERRAIN_GRAPH_PASSES_FINAL_MASK_PASS_H_
