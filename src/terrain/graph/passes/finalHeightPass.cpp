#include "finalHeightPass.h"

namespace pwg
{
	FinalHeightPass::FinalHeightPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize)
		: m_computeShader(computeShader),
		  m_terrainSize(terrainSize)
	{
		m_inputTextures = { "FinalMask" };
		m_outputTextures = { "FinalHeight" };
	}

	void FinalHeightPass::Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx)
	{
		auto finalHeight = masks.at("FinalHeight");
		auto finalMask = masks.at("FinalMask");

		m_computeShader->ActivateShader();

		m_computeShader->SetUniformFloat("u_amplitude", m_amplitude);

		m_computeShader->BindImage(0, finalHeight->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
		m_computeShader->BindImage(1, finalMask->texture->GetTextureID(), GL_READ_ONLY, GL_R32F);
		m_computeShader->DispatchForTexture(m_terrainSize, m_terrainSize);
		m_computeShader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	}

	std::vector<UIParameters> FinalHeightPass::GetParameters()
	{
		return {
			{"Amplitude", ParameterType::Float, &m_amplitude, 0.1, 20000.0}
		};
	}

	std::string FinalHeightPass::GetName()
	{
		return m_name;
	}


}

