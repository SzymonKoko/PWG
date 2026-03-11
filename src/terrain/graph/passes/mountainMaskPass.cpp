#include "mountainMaskPass.h"

namespace pwg
{
	MountainMaskPass::MountainMaskPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize)
		: m_computeShader(computeShader),
		  m_terrainSize(terrainSize)
	{
		m_inputTextures = { "Elevation" };
		m_outputTextures = { "MountainMask" };
	}

	void MountainMaskPass::Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		auto elevationTexture = masks.at("Elevation");
		auto mountainMaskTexture = masks.at("MountainMask");

		m_computeShader->ActivateShader();

		m_computeShader->SetUniformFloat("u_minThreshold", m_parameters.minThreshold);
		m_computeShader->SetUniformFloat("u_maxThreshold", m_parameters.maxThreshold);

		m_computeShader->BindImage(0, mountainMaskTexture->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
		m_computeShader->BindImage(1, elevationTexture->texture->GetTextureID(), GL_READ_ONLY, GL_R32F);
		m_computeShader->DispatchForTexture(m_terrainSize, m_terrainSize);
		m_computeShader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	}

	std::vector<UIParameters> MountainMaskPass::GetParameters()
	{
		return {
			{"Mininimum Threshold", ParameterType::Float, &m_parameters.minThreshold, 0.0, 1.0},
			{"Maximum Threshold", ParameterType::Float, &m_parameters.maxThreshold, 0.0, 1.0}
		};
	}

	std::string MountainMaskPass::GetName()
	{
		return m_name;
	}
}

