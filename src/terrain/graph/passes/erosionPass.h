#ifndef SRC_TERRAIN_GRAPH_PASSES_EROSION_PASS_H_
#define SRC_TERRAIN_GRAPH_PASSES_EROSION_PASS_H_

#include "terrainPass.h"

namespace pwg
{
	struct ErosionPassParameters
	{
		HydraulicSimulationSettings* settings = nullptr;
	};

	class ErosionPass : public TerrainPass
	{
	public:
		ErosionPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize);
		~ErosionPass() = default;

		void SetHydraulicSettings(HydraulicSimulationSettings* settings);
		void Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx) override;
		std::vector<UIParameters> GetParameters() override;
		TerrainPassTypes GetPassType() override;
		std::string GetName() override;
		const std::vector<std::string>& GetInputMasks() const override { return m_inputTextures; }
		const std::vector<std::string>& GetOutputMasks() const override { return m_outputTextures; }

	private:
		std::string m_name = "2. Water + sediment";
		TerrainPassTypes m_passType = TerrainPassTypes::EROSION;
		std::shared_ptr<ComputeShader> m_computeShader;
		std::vector<std::string> m_inputTextures;
		std::vector<std::string> m_outputTextures;
		ErosionPassParameters m_parameters;
		int m_terrainSize = 0;

		std::shared_ptr<TerrainMask> m_tempHeight;
		std::shared_ptr<TerrainMask> m_tempSediment;
		std::shared_ptr<TerrainMask> m_tempDeposit;
	};
}
#endif // !SRC_TERRAIN_GRAPH_PASSES_EROSION_PASS_H_
