#include "hydraulicWaterPass.h"

namespace pwg
{
	HydraulicWaterPass::HydraulicWaterPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize)
		: m_computeShader(computeShader),
		  m_terrainSize(terrainSize)
	{
		m_tempWater = std::make_shared<TerrainMask>("WaterTemp", std::make_shared<Texture>(m_terrainSize, m_terrainSize, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		m_tempFlowAccumulation = std::make_shared<TerrainMask>("FlowAccumulationTemp", std::make_shared<Texture>(m_terrainSize, m_terrainSize, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		m_tempRiverChannels = std::make_shared<TerrainMask>("RiverChannelTemp", std::make_shared<Texture>(m_terrainSize, m_terrainSize, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);

		m_tempWater->texture->SetTextureWrapping(ToGL(TextureWrapMode::CLAMP_TO_EDGE), ToGL(TextureWrapMode::CLAMP_TO_EDGE));
		m_tempFlowAccumulation->texture->SetTextureWrapping(ToGL(TextureWrapMode::CLAMP_TO_EDGE), ToGL(TextureWrapMode::CLAMP_TO_EDGE));
		m_tempRiverChannels->texture->SetTextureWrapping(ToGL(TextureWrapMode::CLAMP_TO_EDGE), ToGL(TextureWrapMode::CLAMP_TO_EDGE));
	}

	void HydraulicWaterPass::SetHydraulicSettings(HydraulicSimulationSettings* settings)
	{
		m_parameters.settings = settings;
	}

	void HydraulicWaterPass::Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx)
	{
		auto finalHeight = masks.at("FinalHeight");
		auto water = masks.at("WaterMask");
		auto flux = masks.at("WaterFlux");
		auto diagonalFlux = masks.at("WaterFluxDiagonal");
		auto flowAccumulation = masks.at("FlowAccumulationMask");
		auto riverChannels = masks.at("RiverChannelMask");

		if (!m_parameters.settings)
		{
			m_parameters.settings = &ctx.hydraulicSettings;
		}

		m_computeShader->ActivateShader();
		m_computeShader->SetUniformInt("u_size", m_terrainSize);
		m_computeShader->SetUniformFloat("u_rainRate", ctx.hydraulicSettings.rainRate);
		m_computeShader->SetUniformFloat("u_rainElevationBias", ctx.hydraulicSettings.rainElevationBias);
		m_computeShader->SetUniformFloat("u_rainMinHeight", ctx.hydraulicSettings.rainMinHeight);
		m_computeShader->SetUniformFloat("u_rainMaxHeight", ctx.hydraulicSettings.rainMaxHeight);
		m_computeShader->SetUniformFloat("u_evaporation", ctx.hydraulicSettings.evaporation);
		m_computeShader->SetUniformFloat("u_infiltrationRate", ctx.hydraulicSettings.infiltrationRate);
		m_computeShader->SetUniformFloat("u_lakeInfiltrationScale", ctx.hydraulicSettings.lakeInfiltrationScale);
		m_computeShader->SetUniformFloat("u_lakeMinDepth", ctx.hydraulicSettings.lakeMinDepth);
		m_computeShader->SetUniformFloat("u_lakeLeveling", ctx.hydraulicSettings.lakeLeveling);
		m_computeShader->SetUniformFloat("u_lakeWaterSmoothing", ctx.hydraulicSettings.lakeWaterSmoothing);
		m_computeShader->SetUniformFloat("u_maxWaterDepth", ctx.hydraulicSettings.maxWaterDepth);
		m_computeShader->SetUniformFloat("u_maxSurfaceDrop", ctx.hydraulicSettings.maxSurfaceDrop);
		m_computeShader->SetUniformFloat("u_minFlowToErode", ctx.hydraulicSettings.minFlowToErode);
		m_computeShader->SetUniformFloat("u_flowAccumulationGrowth", ctx.hydraulicSettings.flowAccumulationGrowth);
		m_computeShader->SetUniformFloat("u_flowAccumulationDecay", ctx.hydraulicSettings.flowAccumulationDecay);
		m_computeShader->SetUniformFloat("u_riverChannelGrowth", ctx.hydraulicSettings.riverChannelGrowth);
		m_computeShader->SetUniformFloat("u_riverChannelDecay", ctx.hydraulicSettings.riverChannelDecay);
		m_computeShader->SetUniformInt("u_height", 0);
		m_computeShader->SetUniformInt("u_waterIn", 1);
		m_computeShader->SetUniformInt("u_flux", 2);
		m_computeShader->SetUniformInt("u_diagonalFlux", 3);
		m_computeShader->SetUniformInt("u_flowAccumulationIn", 4);
		m_computeShader->SetUniformInt("u_riverChannelsIn", 5);

		m_tempWater->texture->ClearTexture(0.0f);
		m_tempFlowAccumulation->texture->ClearTexture(0.0f);
		m_tempRiverChannels->texture->ClearTexture(0.0f);

		m_computeShader->BindTextureSampler(0, finalHeight->texture->GetTextureID());
		m_computeShader->BindTextureSampler(1, water->texture->GetTextureID());
		m_computeShader->BindTextureSampler(2, flux->texture->GetTextureID());
		m_computeShader->BindTextureSampler(3, diagonalFlux->texture->GetTextureID());
		m_computeShader->BindTextureSampler(4, flowAccumulation->texture->GetTextureID());
		m_computeShader->BindTextureSampler(5, riverChannels->texture->GetTextureID());
		m_computeShader->BindImage(0, m_tempWater->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
		m_computeShader->BindImage(1, m_tempFlowAccumulation->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
		m_computeShader->BindImage(2, m_tempRiverChannels->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
		m_computeShader->DispatchForTexture(m_terrainSize, m_terrainSize);
		m_computeShader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

		std::swap(water, m_tempWater);
		std::swap(flowAccumulation, m_tempFlowAccumulation);
		std::swap(riverChannels, m_tempRiverChannels);

		masks["WaterMask"] = water;
		masks["FlowAccumulationMask"] = flowAccumulation;
		masks["RiverChannelMask"] = riverChannels;
	}

	std::vector<UIParameters> HydraulicWaterPass::GetParameters()
	{
		return {};
	}

	TerrainPassTypes HydraulicWaterPass::GetPassType()
	{
		return m_passType;
	}

	std::string HydraulicWaterPass::GetName()
	{
		return m_name;
	}
}
