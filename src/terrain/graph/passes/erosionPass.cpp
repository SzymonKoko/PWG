#include "erosionPass.h"

namespace pwg
{
	ErosionPass::ErosionPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize)
		: m_computeShader(computeShader),
		  m_terrainSize(terrainSize)
	{
		m_inputTextures = { "FinalHeight", "BaseHeight", "WaterMask", "SedimentMask", "DepositMask", "WaterFlux", "WaterFluxDiagonal", "RiverChannelMask", "FlowAccumulationMask" };
		m_outputTextures = { "FinalHeight", "ErodedHeight", "SedimentMask", "DepositMask", "ErosionDelta" };

		m_tempHeight = std::make_shared<TerrainMask>("HeightTemp", std::make_shared<Texture>(m_terrainSize, m_terrainSize, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		m_tempSediment = std::make_shared<TerrainMask>("SedimentTemp", std::make_shared<Texture>(m_terrainSize, m_terrainSize, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		m_tempDeposit = std::make_shared<TerrainMask>("DepositTemp", std::make_shared<Texture>(m_terrainSize, m_terrainSize, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);

		m_tempHeight->texture->SetTextureWrapping(ToGL(TextureWrapMode::CLAMP_TO_EDGE), ToGL(TextureWrapMode::CLAMP_TO_EDGE));
		m_tempSediment->texture->SetTextureWrapping(ToGL(TextureWrapMode::CLAMP_TO_EDGE), ToGL(TextureWrapMode::CLAMP_TO_EDGE));
		m_tempDeposit->texture->SetTextureWrapping(ToGL(TextureWrapMode::CLAMP_TO_EDGE), ToGL(TextureWrapMode::CLAMP_TO_EDGE));
	}

	void ErosionPass::SetHydraulicSettings(HydraulicSimulationSettings* settings)
	{
		m_parameters.settings = settings;
	}

	void ErosionPass::Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx)
	{
		auto finalHeight = masks.at("FinalHeight");
		auto baseHeight = masks.at("BaseHeight");
		auto water = masks.at("WaterMask");
		auto sediment = masks.at("SedimentMask");
		auto deposit = masks.at("DepositMask");
		auto flux = masks.at("WaterFlux");
		auto diagonalFlux = masks.at("WaterFluxDiagonal");
		auto riverChannels = masks.at("RiverChannelMask");
		auto flowAccumulation = masks.at("FlowAccumulationMask");
		auto erosionDelta = masks.at("ErosionDelta");
		if (!m_parameters.settings)
		{
			m_parameters.settings = &ctx.hydraulicSettings;
		}

		m_computeShader->ActivateShader();

		m_computeShader->SetUniformInt("u_size", m_terrainSize);
		m_computeShader->SetUniformFloat("u_sedimentCapacity", ctx.hydraulicSettings.sedimentCapacity);
		m_computeShader->SetUniformFloat("u_erosionStrength", ctx.hydraulicSettings.erosionStrength);
		m_computeShader->SetUniformFloat("u_depositionStrength", ctx.hydraulicSettings.depositionStrength);
		m_computeShader->SetUniformFloat("u_minWaterToErode", ctx.hydraulicSettings.minWaterToErode);
		m_computeShader->SetUniformFloat("u_minFlowToErode", ctx.hydraulicSettings.minFlowToErode);
		m_computeShader->SetUniformFloat("u_maxHeightChange", ctx.hydraulicSettings.maxHeightChange);
		m_computeShader->SetUniformFloat("u_maxErosionDepth", ctx.hydraulicSettings.maxErosionDepth);
		m_computeShader->SetUniformFloat("u_maxDepositHeight", ctx.hydraulicSettings.maxDepositHeight);
		m_computeShader->SetUniformFloat("u_riverChannelInfluence", ctx.hydraulicSettings.riverChannelInfluence);
		m_computeShader->SetUniformFloat("u_riverChannelDepth", ctx.hydraulicSettings.riverChannelDepth);
		m_computeShader->SetUniformFloat("u_flatteningStrength", ctx.hydraulicSettings.flatteningStrength);
		m_computeShader->SetUniformInt("u_heightIn", 0);
		m_computeShader->SetUniformInt("u_baseHeight", 1);
		m_computeShader->SetUniformInt("u_waterIn", 2);
		m_computeShader->SetUniformInt("u_sedimentIn", 3);
		m_computeShader->SetUniformInt("u_flux", 4);
		m_computeShader->SetUniformInt("u_diagonalFlux", 5);
		m_computeShader->SetUniformInt("u_riverChannelsIn", 6);
		m_computeShader->SetUniformInt("u_flowAccumulationIn", 7);
		m_computeShader->SetUniformInt("u_depositIn", 8);

		// Keep the previous state as the default output. If a shader compile/dispatch
		// issue ever prevents writes, we preserve terrain instead of swapping in zeros.
		m_tempHeight->texture->CopyFrom(*finalHeight->texture);
		m_tempSediment->texture->CopyFrom(*sediment->texture);
		m_tempDeposit->texture->CopyFrom(*deposit->texture);
		erosionDelta->texture->ClearTexture(0.0f);

		m_computeShader->BindTextureSampler(0, finalHeight->texture->GetTextureID());
		m_computeShader->BindTextureSampler(1, baseHeight->texture->GetTextureID());
		m_computeShader->BindTextureSampler(2, water->texture->GetTextureID());
		m_computeShader->BindTextureSampler(3, sediment->texture->GetTextureID());
		m_computeShader->BindTextureSampler(4, flux->texture->GetTextureID());
		m_computeShader->BindTextureSampler(5, diagonalFlux->texture->GetTextureID());
		m_computeShader->BindTextureSampler(6, riverChannels->texture->GetTextureID());
		m_computeShader->BindTextureSampler(7, flowAccumulation->texture->GetTextureID());
		m_computeShader->BindTextureSampler(8, deposit->texture->GetTextureID());
		m_computeShader->BindImage(0, m_tempHeight->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
		m_computeShader->BindImage(1, m_tempSediment->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
		m_computeShader->BindImage(2, m_tempDeposit->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
		m_computeShader->BindImage(3, erosionDelta->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
		m_computeShader->DispatchForTexture(m_terrainSize, m_terrainSize);
		m_computeShader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

		std::swap(finalHeight, m_tempHeight);
		std::swap(sediment, m_tempSediment);
		std::swap(deposit, m_tempDeposit);

		masks["FinalHeight"] = finalHeight;
		masks["ErodedHeight"] = finalHeight;
		masks["SedimentMask"] = sediment;
		masks["DepositMask"] = deposit;
	}

	std::vector<UIParameters> ErosionPass::GetParameters()
	{
		if (!m_parameters.settings)
		{
			return {};
		}

		return {
			{"Enable Erosion", ParameterType::Bool, &m_parameters.settings->erosionEnabled, 0, 1},
			{"Auto-run simulation", ParameterType::Bool, &m_parameters.settings->autoWaterSimulation, 0, 1},
			{"Show Water Surface", ParameterType::Bool, &m_parameters.settings->showWaterSurface, 0, 1},
			{"Use Water Refraction", ParameterType::Bool, &m_parameters.settings->waterRefractionEnabled, 0, 1},
			{"Sim - steps per click", ParameterType::Int, &m_parameters.settings->stepsPerUpdate, 1, 32},
			{"Sim - steps per second", ParameterType::Int, &m_parameters.settings->waterStepsPerSecond, 1, 60},
			{"Sim - substeps", ParameterType::Int, &m_parameters.settings->waterSubsteps, 1, 8},
			{"Erosion - channel carving", ParameterType::Float, &m_parameters.settings->erosionStrength, 0.0f, 0.6f},
			{"Erosion - terrain flattening", ParameterType::Float, &m_parameters.settings->flatteningStrength, 0.0f, 1.0f},
			{"Erosion - sediment capacity", ParameterType::Float, &m_parameters.settings->sedimentCapacity, 0.5f, 20.0f},
			{"Sediment - cosmetic deposit", ParameterType::Float, &m_parameters.settings->depositionStrength, 0.0f, 0.08f},
			{"Sediment - max deposit height", ParameterType::Float, &m_parameters.settings->maxDepositHeight, 0.01f, 0.5f},
			{"Limits - max change/step", ParameterType::Float, &m_parameters.settings->maxHeightChange, 0.001f, 0.1f},
			{"Limits - max cut depth", ParameterType::Float, &m_parameters.settings->maxErosionDepth, 0.01f, 2.0f},
			{"Rivers - channel depth", ParameterType::Float, &m_parameters.settings->riverChannelDepth, 0.0f, 0.3f},
			{"Rivers - channel influence", ParameterType::Float, &m_parameters.settings->riverChannelInfluence, 0.0f, 1.0f}
		};
	}

	TerrainPassTypes ErosionPass::GetPassType()
	{
		return m_passType;
	}

	std::string ErosionPass::GetName()
	{
		return m_name;
	}
}
