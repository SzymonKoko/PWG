#include "normalMaskPass.h"

namespace pwg
{
	NormalMaskPass::NormalMaskPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize)
		: m_computeShader(computeShader),
		  m_terrainSize(terrainSize)
	{
		m_inputTextures = { "FinalHeight" };
		m_outputTextures = { "NormalMask" };
	}

	void NormalMaskPass::Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		auto finalHeight = masks.at("FinalHeight");
		auto normalMask = masks.at("NormalMask");

		m_computeShader->ActivateShader();

		m_computeShader->BindImage(0, normalMask->texture->GetTextureID(), GL_WRITE_ONLY, GL_RGBA16F);
		m_computeShader->BindImage(1, finalHeight->texture->GetTextureID(), GL_READ_ONLY, GL_R32F);
		m_computeShader->DispatchForTexture(m_terrainSize, m_terrainSize);
		m_computeShader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	}

	std::vector<UIParameters> NormalMaskPass::GetParameters()
	{
		return {};
	}

	std::string NormalMaskPass::GetName()
	{
		return m_name;
	}
}


