#ifndef SRC_TERRAIN_GRAPH_PASSES_FINAL_HEIGHT_PASS_H_
#define SRC_TERRAIN_GRAPH_PASSES_FINAL_HEIGHT_PASS_H_

#include "terrainPass.h"

namespace pwg
{
	class FinalHeightPass : public TerrainPass
	{
	public:
		FinalHeightPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize);
		~FinalHeightPass() = default;

		void Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx) override;
		std::vector<UIParameters> GetParameters() override;
		std::string GetName() override;

	private:
		std::string m_name = "FinalHeightPass";
		TerrainPassTypes m_passType = TerrainPassTypes::GENERATION;
		std::shared_ptr<ComputeShader> m_computeShader;
		std::vector<std::string> m_inputTextures;
		std::vector<std::string> m_outputTextures;
		float m_amplitude{ 0 };
		int m_terrainSize{ 0 };
	};
}
#endif // !SRC_TERRAIN_GRAPH_PASSES_FINAL_HEIGHT_PASS_H_
