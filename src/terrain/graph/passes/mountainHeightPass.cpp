#include "mountainHeightPass.h"

namespace pwg
{
	MountainHeightPass::MountainHeightPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize)
		: m_computeShader(computeShader),
		  m_terrainSize(terrainSize)
	{
		m_inputTextures = { "MountainMask" };
		m_outputTextures = { "MountainHeight" };
	}

	void MountainHeightPass::Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx)
	{
		auto mountainHeight = masks.at("MountainHeight");
		auto mountainMask = masks.at("MountainMask");

		m_computeShader->ActivateShader();

		m_computeShader->SetUniformInt("u_noiseType", m_parameters.noiseType);
		m_computeShader->SetUniformFloat("u_amplitude", m_parameters.amplitude);
		m_computeShader->SetUniformFloat("u_frequency", m_parameters.frequency);
		m_computeShader->SetUniformFloat("u_scale", m_parameters.scale);
		m_computeShader->SetUniformVec2("u_offset", ctx.offset);
		m_computeShader->SetUniformInt("u_seed", m_parameters.seed);

		m_computeShader->SetUniformInt("u_fractalType", m_parameters.fractalType);
		m_computeShader->SetUniformInt("u_octaves", m_parameters.octaves);
		m_computeShader->SetUniformFloat("u_lacunarity", m_parameters.lacunarity);
		m_computeShader->SetUniformFloat("u_persistance", m_parameters.persistance);

		m_computeShader->SetUniformFloat("u_ridgeStrength", m_parameters.ridgeStrength);

		m_computeShader->SetUniformBool("u_falloff", m_parameters.falloff);
		m_computeShader->SetUniformFloat("u_falloffStrength", m_parameters.falloffStrength);

		m_computeShader->SetUniformInt("u_size", m_terrainSize);

		m_computeShader->BindImage(0, mountainHeight->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
		m_computeShader->BindImage(1, mountainMask->texture->GetTextureID(), GL_READ_ONLY, GL_R32F);
		m_computeShader->DispatchForTexture(m_terrainSize, m_terrainSize);
		m_computeShader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	}
	std::vector<UIParameters> MountainHeightPass::GetParameters()
	{
		return {
			{"Noise Type", ParameterType::List, &m_parameters.noiseType, 0, 3},
			{"Frequency", ParameterType::Float, &m_parameters.frequency, 0.01, 2.0},
			{"Amplitude", ParameterType::Float, &m_parameters.amplitude, 0.01, 200.0},
			{"Scale", ParameterType::Float, &m_parameters.scale, 0.01, 200.0f},
			{"Seed", ParameterType::Int, &m_parameters.seed, 1, 200},
			{"Fractal Type", ParameterType::List, &m_parameters.fractalType, 0, 2},
			{"Octaves", ParameterType::Int, &m_parameters.octaves, 1, 8},
			{"Lacunarity", ParameterType::Float, &m_parameters.lacunarity, 0.0, 4.0},
			{"Persistance", ParameterType::Float, &m_parameters.persistance, 0.0, 1.0},
			{"Ridge Strength", ParameterType::Float, &m_parameters.ridgeStrength, 0.0, 3.0},
			{"Falloff Enabled", ParameterType::Bool, &m_parameters.falloff, 0, 1},
			{"Falloff Strength", ParameterType::Float, &m_parameters.falloffStrength, 0.0, 3.0}
		};
	}
	std::string MountainHeightPass::GetName()
	{
		return m_name;
	}
}

