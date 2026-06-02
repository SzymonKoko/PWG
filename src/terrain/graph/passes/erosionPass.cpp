#include "erosionPass.h"

namespace pwg
{
	ErosionPass::ErosionPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize)
		: m_computeShader(computeShader),
		  m_terrainSize(terrainSize)
	{
		m_inputTextures = { "FinalHeight", "BaseHeight", "WaterMask", "SedimentMask", "DepositMask", "WaterFlux", "WaterFluxDiagonal", "RiverChannelMask", "FlowAccumulationMask" };
		m_outputTextures = { "ErodedHeight", "SedimentMask", "DepositMask", "ErosionDelta" };

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
		m_computeShader->SetUniformFloat("u_capacity", ctx.hydraulicSettings.sedimentCapacity);
		m_computeShader->SetUniformFloat("u_erosionRate", ctx.hydraulicSettings.erosionRate);
		m_computeShader->SetUniformFloat("u_slopeErosionRate", ctx.hydraulicSettings.slopeErosionRate);
		m_computeShader->SetUniformFloat("u_slopeErosionMinSlope", ctx.hydraulicSettings.slopeErosionMinSlope);
		m_computeShader->SetUniformFloat("u_slopeErosionWaterScale", ctx.hydraulicSettings.slopeErosionWaterScale);
		m_computeShader->SetUniformFloat("u_depositionRate", ctx.hydraulicSettings.depositionRate);
		m_computeShader->SetUniformFloat("u_valleyDepositionRate", ctx.hydraulicSettings.valleyDepositionRate);
		m_computeShader->SetUniformFloat("u_valleyDepositionSlope", ctx.hydraulicSettings.valleyDepositionSlope);
		m_computeShader->SetUniformFloat("u_steepSlopeDepositionReduction", ctx.hydraulicSettings.steepSlopeDepositionReduction);
		m_computeShader->SetUniformFloat("u_depositionSlopeStart", ctx.hydraulicSettings.depositionSlopeStart);
		m_computeShader->SetUniformFloat("u_depositionSlopeEnd", ctx.hydraulicSettings.depositionSlopeEnd);
		m_computeShader->SetUniformFloat("u_minWaterToErode", ctx.hydraulicSettings.minWaterToErode);
		m_computeShader->SetUniformFloat("u_minFlowToErode", ctx.hydraulicSettings.minFlowToErode);
		m_computeShader->SetUniformFloat("u_maxHeightChange", ctx.hydraulicSettings.maxHeightChange);
		m_computeShader->SetUniformFloat("u_maxErosionDepth", ctx.hydraulicSettings.maxErosionDepth);
		m_computeShader->SetUniformFloat("u_maxDepositionHeight", ctx.hydraulicSettings.maxDepositionHeight);
		m_computeShader->SetUniformFloat("u_lakeDeposition", ctx.hydraulicSettings.lakeDeposition);
		m_computeShader->SetUniformFloat("u_lakeMinDepth", ctx.hydraulicSettings.lakeMinDepth);
		m_computeShader->SetUniformFloat("u_lakeBedSmoothing", ctx.hydraulicSettings.lakeBedSmoothing);
		m_computeShader->SetUniformFloat("u_channelSmoothing", ctx.hydraulicSettings.channelSmoothing);
		m_computeShader->SetUniformFloat("u_riverChannelInfluence", ctx.hydraulicSettings.riverChannelInfluence);
		m_computeShader->SetUniformFloat("u_riverChannelDepth", ctx.hydraulicSettings.riverChannelDepth);
		m_computeShader->SetUniformInt("u_heightIn", 0);
		m_computeShader->SetUniformInt("u_baseHeight", 1);
		m_computeShader->SetUniformInt("u_waterIn", 2);
		m_computeShader->SetUniformInt("u_sedimentIn", 3);
		m_computeShader->SetUniformInt("u_flux", 4);
		m_computeShader->SetUniformInt("u_diagonalFlux", 5);
		m_computeShader->SetUniformInt("u_riverChannelsIn", 6);
		m_computeShader->SetUniformInt("u_flowAccumulationIn", 7);
		m_computeShader->SetUniformInt("u_depositIn", 8);

		m_tempHeight->texture->ClearTexture(0.0f);
		m_tempSediment->texture->ClearTexture(0.0f);
		m_tempDeposit->texture->ClearTexture(0.0f);
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
			{"Auto Water Physics", ParameterType::Bool, &m_parameters.settings->autoWaterSimulation, 0, 1},
			{"Show Water Surface", ParameterType::Bool, &m_parameters.settings->showWaterSurface, 0, 1},
			{"Use Water Refraction", ParameterType::Bool, &m_parameters.settings->waterRefractionEnabled, 0, 1},
			{"Sim - steps per click", ParameterType::Int, &m_parameters.settings->stepsPerUpdate, 1, 32},
			{"Sim - water steps/sec", ParameterType::Int, &m_parameters.settings->waterStepsPerSecond, 1, 120},
			{"Sim - water substeps", ParameterType::Int, &m_parameters.settings->waterSubsteps, 1, 8},
			{"Rain - amount per step", ParameterType::Float, &m_parameters.settings->rainRate, 0.0f, 0.05f},
			{"Water - evaporates each step", ParameterType::Float, &m_parameters.settings->evaporation, 0.0f, 0.05f},
			{"Water - soaks into ground", ParameterType::Float, &m_parameters.settings->infiltrationRate, 0.0f, 0.03f},
			{"Water - lake soaking multiplier", ParameterType::Float, &m_parameters.settings->lakeInfiltrationScale, 0.0f, 1.0f},
			{"Water - max depth clamp", ParameterType::Float, &m_parameters.settings->maxWaterDepth, 0.1f, 10.0f},
			{"Sediment - transport capacity", ParameterType::Float, &m_parameters.settings->sedimentCapacity, 0.1f, 20.0f},
			{"Erosion - river/channel carving", ParameterType::Float, &m_parameters.settings->erosionRate, 0.0f, 1.0f},
			{"Erosion - slope wash strength", ParameterType::Float, &m_parameters.settings->slopeErosionRate, 0.0f, 0.5f},
			{"Erosion - slope wash starts", ParameterType::Float, &m_parameters.settings->slopeErosionMinSlope, 0.0f, 0.12f},
			{"Erosion - rain/water boost", ParameterType::Float, &m_parameters.settings->slopeErosionWaterScale, 0.0f, 2.0f},
			{"Deposition - general settling", ParameterType::Float, &m_parameters.settings->depositionRate, 0.0f, 0.3f},
			{"Deposition - valley/alluvial boost", ParameterType::Float, &m_parameters.settings->valleyDepositionRate, 0.0f, 0.4f},
			{"Deposition - valley max slope", ParameterType::Float, &m_parameters.settings->valleyDepositionSlope, 0.0f, 0.3f},
			{"Deposition - less on fast slopes", ParameterType::Float, &m_parameters.settings->steepSlopeDepositionReduction, 0.0f, 1.0f},
			{"Deposition - slope reduction starts", ParameterType::Float, &m_parameters.settings->depositionSlopeStart, 0.0f, 0.3f},
			{"Deposition - slope reduction full", ParameterType::Float, &m_parameters.settings->depositionSlopeEnd, 0.01f, 0.6f},
			{"Erosion - minimum water", ParameterType::Float, &m_parameters.settings->minWaterToErode, 0.0f, 0.2f},
			{"Erosion - minimum flow", ParameterType::Float, &m_parameters.settings->minFlowToErode, 0.0f, 0.2f},
			{"Limits - max terrain change/step", ParameterType::Float, &m_parameters.settings->maxHeightChange, 0.001f, 0.2f},
			{"Limits - max cut below base", ParameterType::Float, &m_parameters.settings->maxErosionDepth, 0.01f, 2.0f},
			{"Limits - max fill above base", ParameterType::Float, &m_parameters.settings->maxDepositionHeight, 0.01f, 2.0f},
			{"Lakes - extra sediment settling", ParameterType::Float, &m_parameters.settings->lakeDeposition, 0.0f, 1.0f},
			{"Lakes - min pooled depth", ParameterType::Float, &m_parameters.settings->lakeMinDepth, 0.01f, 0.5f},
			{"Lakes - water leveling", ParameterType::Float, &m_parameters.settings->lakeLeveling, 0.0f, 0.5f},
			{"Lakes - smooth lake bed", ParameterType::Float, &m_parameters.settings->lakeBedSmoothing, 0.0f, 0.3f},
			{"Lakes - smooth water surface", ParameterType::Float, &m_parameters.settings->lakeWaterSmoothing, 0.0f, 0.5f},
			{"Debug - flow accumulation growth", ParameterType::Float, &m_parameters.settings->flowAccumulationGrowth, 0.0f, 1.0f},
			{"Debug - flow accumulation decay", ParameterType::Float, &m_parameters.settings->flowAccumulationDecay, 0.0f, 0.3f},
			{"Channels - smooth carved beds", ParameterType::Float, &m_parameters.settings->channelSmoothing, 0.0f, 0.5f}
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
