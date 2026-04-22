#include "elevationPass.h"

namespace pwg
{
	ElevationPass::ElevationPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize)
		: m_computeShader(computeShader),
		  m_terrainSize(terrainSize)
	{
		m_inputTextures = {};
		m_outputTextures = { "Elevation" };
	}

	void ElevationPass::Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx)
	{
		auto elevationTexture = masks.at("Elevation");

		m_computeShader->ActivateShader();

		m_computeShader->SetUniformInt("u_noiseType", m_parameters.noiseType);
		m_computeShader->SetUniformInt("u_seed", m_parameters.seed);
		m_computeShader->SetUniformInt("u_size", m_terrainSize);
		m_computeShader->SetUniformFloat("u_frequency", m_parameters.frequency);
		m_computeShader->SetUniformFloat("u_scale", m_parameters.scale);
		m_computeShader->SetUniformVec2("u_offset", ctx.offset);

		m_computeShader->BindImage(0, elevationTexture->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
		m_computeShader->DispatchForTexture(m_terrainSize, m_terrainSize);
		m_computeShader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	}

	std::vector<UIParameters> ElevationPass::GetParameters()
	{
		return {
			{"Noise Type", ParameterType::List, &m_parameters.noiseType, 0, 3},
			{"Seed", ParameterType::Int, &m_parameters.seed, 1, 200},
			{"Frequency", ParameterType::Float, &m_parameters.frequency, 0.01, 5.0},
			{"Scale", ParameterType::Float, &m_parameters.scale, 0.01, 200.0f}
		};
	}

	std::string ElevationPass::GetName()
	{
		return m_name; 
	}
}


