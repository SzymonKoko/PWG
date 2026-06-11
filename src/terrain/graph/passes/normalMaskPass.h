#ifndef SRC_TERRAIN_GRAPH_PASSES_NORMAL_MASK_PASS_H_
#define SRC_TERRAIN_GRAPH_PASSES_NORMAL_MASK_PASS_H_

#include "terrainPass.h"

namespace pwg
{
	class NormalMaskPass : public TerrainPass
	{
	public:
		NormalMaskPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize);
		~NormalMaskPass() = default;

		void Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx) override;
		std::vector<UIParameters> GetParameters() override;
		TerrainPassTypes GetPassType() override;
		std::string GetName() override;
		const std::vector<std::string>& GetInputMasks() const override { return m_inputTextures; }
		const std::vector<std::string>& GetOutputMasks() const override { return m_outputTextures; }

	private:
		std::string m_name = "NormalMaskPass";
		TerrainPassTypes m_passType = TerrainPassTypes::MASK;
		std::shared_ptr<ComputeShader> m_computeShader;
		std::vector<std::string> m_inputTextures;
		std::vector<std::string> m_outputTextures;
		int m_terrainSize = 0;
	};
}
#endif // !SRC_TERRAIN_GRAPH_PASSES_NORMAL_MASK_PASS_H_
