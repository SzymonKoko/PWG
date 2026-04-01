#include "slopeMaskPass.h"

namespace pwg
{
	SlopeMaskPass::SlopeMaskPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize)
		: m_computeShader(computeShader),
		  m_terrainSize(terrainSize)
	{
		m_inputTextures = { "NormalMask" };
		m_outputTextures = { "SlopeMask" };
	}
	void SlopeMaskPass::Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx)
	{
		auto normalMask = masks.at("NormalMask");
		auto slopeMask = masks.at("SlopeMask");

		m_computeShader->ActivateShader();

		m_computeShader->BindImage(0, slopeMask->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
		m_computeShader->BindImage(1, normalMask->texture->GetTextureID(), GL_READ_ONLY, GL_RGBA16F);
		m_computeShader->DispatchForTexture(m_terrainSize, m_terrainSize);
		m_computeShader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	}
	std::vector<UIParameters> SlopeMaskPass::GetParameters()
	{
		return { };
	}
	std::string SlopeMaskPass::GetName()
	{
		return m_name;
	}
}

