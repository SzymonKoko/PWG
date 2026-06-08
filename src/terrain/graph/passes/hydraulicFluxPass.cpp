#include "hydraulicFluxPass.h"

namespace pwg
{
	HydraulicFluxPass::HydraulicFluxPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize)
		: m_computeShader(computeShader),
		  m_terrainSize(terrainSize)
	{
		m_inputTextures = { "FinalHeight", "WaterMask", "RiverChannelMask" };
		m_outputTextures = { "WaterFlux", "WaterFluxDiagonal" };

		m_tempFlux = std::make_shared<TerrainMask>("WaterFluxTemp", std::make_shared<Texture>(m_terrainSize, m_terrainSize, TextureType::HEIGHT4), MaskUsage::WEIGHTS, 1.0f);
		m_tempDiagonalFlux = std::make_shared<TerrainMask>("WaterFluxDiagonalTemp", std::make_shared<Texture>(m_terrainSize, m_terrainSize, TextureType::HEIGHT4), MaskUsage::WEIGHTS, 1.0f);
		m_tempFlux->texture->SetTextureWrapping(ToGL(TextureWrapMode::CLAMP_TO_EDGE), ToGL(TextureWrapMode::CLAMP_TO_EDGE));
		m_tempDiagonalFlux->texture->SetTextureWrapping(ToGL(TextureWrapMode::CLAMP_TO_EDGE), ToGL(TextureWrapMode::CLAMP_TO_EDGE));
	}

	void HydraulicFluxPass::SetHydraulicSettings(HydraulicSimulationSettings* settings)
	{
		m_parameters.settings = settings;
	}

	void HydraulicFluxPass::Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx)
	{
		auto finalHeight = masks.at("FinalHeight");
		auto water = masks.at("WaterMask");
		auto riverChannels = masks.at("RiverChannelMask");
		auto flux = masks.at("WaterFlux");
		auto diagonalFlux = masks.at("WaterFluxDiagonal");
		if (!m_parameters.settings)
		{
			m_parameters.settings = &ctx.hydraulicSettings;
		}

		m_tempFlux->texture->ClearTexture(0.0f);
		m_tempDiagonalFlux->texture->ClearTexture(0.0f);

		m_computeShader->ActivateShader();
		m_computeShader->SetUniformInt("u_size", m_terrainSize);
		m_computeShader->SetUniformFloat("u_pipeFlow", ctx.hydraulicSettings.pipeFlow);
		m_computeShader->SetUniformFloat("u_pipeDamping", ctx.hydraulicSettings.pipeDamping);
		m_computeShader->SetUniformFloat("u_rainRate", ctx.hydraulicSettings.rainRate);
		m_computeShader->SetUniformFloat("u_rainElevationBias", ctx.hydraulicSettings.rainElevationBias);
		m_computeShader->SetUniformFloat("u_rainMinHeight", ctx.hydraulicSettings.rainMinHeight);
		m_computeShader->SetUniformFloat("u_rainMaxHeight", ctx.hydraulicSettings.rainMaxHeight);
		m_computeShader->SetUniformFloat("u_boundaryDrain", ctx.hydraulicSettings.boundaryDrain);
		m_computeShader->SetUniformFloat("u_maxWaterDepth", ctx.hydraulicSettings.maxWaterDepth);
		m_computeShader->SetUniformFloat("u_riverChannelInfluence", ctx.hydraulicSettings.riverChannelInfluence);
		m_computeShader->SetUniformFloat("u_riverChannelDepth", ctx.hydraulicSettings.riverChannelDepth);
		m_computeShader->SetUniformFloat("u_flowConcentration", ctx.hydraulicSettings.flowConcentration);
		m_computeShader->SetUniformInt("u_useDiagonalFlow", ctx.hydraulicSettings.useDiagonalFlow);
		m_computeShader->SetUniformInt("u_height", 0);
		m_computeShader->SetUniformInt("u_water", 1);
		m_computeShader->SetUniformInt("u_previousFlux", 2);
		m_computeShader->SetUniformInt("u_previousDiagonalFlux", 3);
		m_computeShader->SetUniformInt("u_riverChannels", 4);

		m_computeShader->BindTextureSampler(0, finalHeight->texture->GetTextureID());
		m_computeShader->BindTextureSampler(1, water->texture->GetTextureID());
		m_computeShader->BindTextureSampler(2, flux->texture->GetTextureID());
		m_computeShader->BindTextureSampler(3, diagonalFlux->texture->GetTextureID());
		m_computeShader->BindTextureSampler(4, riverChannels->texture->GetTextureID());
		m_computeShader->BindImage(0, m_tempFlux->texture->GetTextureID(), GL_WRITE_ONLY, GL_RGBA32F);
		m_computeShader->BindImage(1, m_tempDiagonalFlux->texture->GetTextureID(), GL_WRITE_ONLY, GL_RGBA32F);
		m_computeShader->DispatchForTexture(m_terrainSize, m_terrainSize);
		m_computeShader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

		std::swap(flux, m_tempFlux);
		std::swap(diagonalFlux, m_tempDiagonalFlux);

		masks["WaterFlux"] = flux;
		masks["WaterFluxDiagonal"] = diagonalFlux;
	}

	std::vector<UIParameters> HydraulicFluxPass::GetParameters()
	{
		if (!m_parameters.settings)
		{
			return {};
		}

		return {
			{"Rain - amount per step", ParameterType::Float, &m_parameters.settings->rainRate, 0.0f, 0.02f},
			{"Rain - high terrain boost", ParameterType::Float, &m_parameters.settings->rainElevationBias, 0.0f, 1.0f},
			{"Pipe - flow speed", ParameterType::Float, &m_parameters.settings->pipeFlow, 0.05f, 0.95f},
			{"Pipe - direction memory", ParameterType::Float, &m_parameters.settings->pipeDamping, 0.0f, 0.95f},
			{"Flow - river concentration", ParameterType::Float, &m_parameters.settings->flowConcentration, 0.0f, 0.85f},
			{"Flow - diagonal neighbors", ParameterType::Bool, &m_parameters.settings->useDiagonalFlow, 0, 1},
			{"Flow - map edge drain", ParameterType::Float, &m_parameters.settings->boundaryDrain, 0.0f, 1.0f},
			{"Water - max depth", ParameterType::Float, &m_parameters.settings->maxWaterDepth, 0.2f, 6.0f}
		};
	}

	TerrainPassTypes HydraulicFluxPass::GetPassType()
	{
		return m_passType;
	}

	std::string HydraulicFluxPass::GetName()
	{
		return m_name;
	}
}
