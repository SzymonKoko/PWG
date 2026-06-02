#include "terrainComputePipeline.h"
#include <algorithm>

namespace pwg
{
	TerrainComputePipeline::TerrainComputePipeline(int terrainSize, std::shared_ptr<ShaderManager> shaderManager, std::shared_ptr<TerrainDebug> debug)
		: m_size(terrainSize),
		  m_shaderManager(shaderManager)
	{
		m_size = terrainSize;
		m_terrainPassContext.chunkSize = terrainSize;
		m_graph = std::make_shared<TerrainGraph>();

		std::string terrainShadersPath = "../assets/shaders/terrainShaders/";

		m_shaderManager->LoadCompute("elevationMaskShader", terrainShadersPath + "masks/elevationMask.comp");
		m_shaderManager->LoadCompute("mountainMaskShader", terrainShadersPath + "masks/mountainMask.comp");
		m_shaderManager->LoadCompute("mountainHeightShader", terrainShadersPath + "masks/mountainHeight.comp");
		m_shaderManager->LoadCompute("finalMaskShader", terrainShadersPath + "masks/finalMask.comp");
		m_shaderManager->LoadCompute("finalHeightShader", terrainShadersPath + "masks/finalHeight.comp");
		m_shaderManager->LoadCompute("normalMaskShader", terrainShadersPath + "masks/normalMask.comp");
		m_shaderManager->LoadCompute("slopeMaskShader", terrainShadersPath + "masks/slopeMask.comp");
		m_shaderManager->LoadCompute("hydraulicFluxShader", terrainShadersPath + "masks/hydraulicFlux.comp");
		m_shaderManager->LoadCompute("hydraulicWaterShader", terrainShadersPath + "masks/hydraulicWater.comp");
		m_shaderManager->LoadCompute("erosionMaskShader", terrainShadersPath + "masks/erosionMask.comp");

		auto elevationShader = m_shaderManager->GetShader<pwg::ComputeShader>("elevationMaskShader");
		auto mountainMaskShader = m_shaderManager->GetShader<pwg::ComputeShader>("mountainMaskShader");
		auto mountainHeightShader = m_shaderManager->GetShader<pwg::ComputeShader>("mountainHeightShader");
		auto finalMaskShader = m_shaderManager->GetShader<pwg::ComputeShader>("finalMaskShader");
		auto finalHeightShader = m_shaderManager->GetShader<pwg::ComputeShader>("finalHeightShader");
		auto normalMaskShader = m_shaderManager->GetShader<pwg::ComputeShader>("normalMaskShader");
		auto slopeMaskShader = m_shaderManager->GetShader<pwg::ComputeShader>("slopeMaskShader");
		auto hydraulicFluxShader = m_shaderManager->GetShader<pwg::ComputeShader>("hydraulicFluxShader");
		auto hydraulicWaterShader = m_shaderManager->GetShader<pwg::ComputeShader>("hydraulicWaterShader");
		auto erosionMaskShader = m_shaderManager->GetShader<pwg::ComputeShader>("erosionMaskShader");

		AddComputeShader("Elevation", elevationShader);
		AddComputeShader("MountainMask", mountainMaskShader);
		AddComputeShader("MountainHeight", mountainHeightShader);
		AddComputeShader("FinalMask", finalMaskShader);
		AddComputeShader("FinalHeight", finalHeightShader);
		AddComputeShader("NormalMask", normalMaskShader);
		AddComputeShader("SlopeMask", slopeMaskShader);
		AddComputeShader("HydraulicFlux", hydraulicFluxShader);
		AddComputeShader("HydraulicWater", hydraulicWaterShader);
		AddComputeShader("ErosionMask", erosionMaskShader);
	}

	TerrainComputePipeline::~TerrainComputePipeline()
	{
	}

	void TerrainComputePipeline::Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		RegenerateTerrain(masks);
	}

	void TerrainComputePipeline::RegenerateTerrain(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		m_terrainPassContext.offset = glm::vec2(0, 0);
		m_terrainPassContext.deferParticleTextureUpload = false;
		m_graph->Execute(masks, m_terrainPassContext);
		masks.at("BaseHeight")->texture->CopyFrom(*masks.at("FinalHeight")->texture);
		m_terrainPassContext.cpuTerrainCacheValid = false;
		m_terrainPassContext.cpuHeight.clear();
		m_terrainPassContext.cpuBaseHeight.clear();
		m_terrainPassContext.cpuDeposit.clear();
		m_terrainPassContext.cpuWater.clear();
		m_terrainPassContext.cpuSediment.clear();
		m_terrainPassContext.cpuFlowAccumulation.clear();
		m_terrainPassContext.cpuRiverChannels.clear();
		m_terrainPassContext.cpuErosionDelta.clear();
		m_terrainMasks = masks;
	}

	void TerrainComputePipeline::StepErosionSimulation(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		m_terrainPassContext.offset = glm::vec2(0, 0);
		m_terrainPassContext.deferParticleTextureUpload = false;
		for (int i = 0; i < m_terrainPassContext.hydraulicSettings.stepsPerUpdate; i++)
		{
			if (m_terrainPassContext.hydraulicSettings.erosionEnabled)
			{
				m_graph->ExecutePasses(TerrainPassTypes::EROSION, masks, m_terrainPassContext);
			}
		}
		m_graph->ExecutePasses(TerrainPassTypes::MASK, masks, m_terrainPassContext);
		m_terrainMasks = masks;
	}

	void TerrainComputePipeline::StepParticleBake(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, bool uploadPreview)
	{
		m_terrainPassContext.offset = glm::vec2(0, 0);
		m_terrainPassContext.deferParticleTextureUpload = !uploadPreview;
		if (m_terrainPassContext.hydraulicSettings.erosionEnabled)
		{
			m_graph->ExecutePasses(TerrainPassTypes::EROSION, masks, m_terrainPassContext);
		}
		m_terrainPassContext.deferParticleTextureUpload = false;

		if (uploadPreview)
		{
			m_graph->ExecutePasses(TerrainPassTypes::MASK, masks, m_terrainPassContext);
		}
		m_terrainMasks = masks;
	}

	void TerrainComputePipeline::BakeParticleErosion(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		m_terrainPassContext.offset = glm::vec2(0, 0);
		int bakeSteps = std::max(m_terrainPassContext.hydraulicSettings.bakeParticleSteps, 1);
		for (int i = 0; i < bakeSteps; ++i)
		{
			StepParticleBake(masks, i == bakeSteps - 1);
		}
		m_terrainMasks = masks;
	}

	void TerrainComputePipeline::StepWaterSimulation(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		m_terrainPassContext.offset = glm::vec2(0, 0);
		for (int i = 0; i < m_terrainPassContext.hydraulicSettings.stepsPerUpdate; i++)
		{
			m_graph->ExecutePasses(TerrainPassTypes::HYDROLOGY, masks, m_terrainPassContext);
		}
		m_terrainMasks = masks;
	}

	void TerrainComputePipeline::ResetWaterSimulation(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		m_terrainPassContext.deferParticleTextureUpload = false;
		masks.at("WaterMask")->texture->ClearTexture();
		masks.at("WaterFlux")->texture->ClearTexture();
		masks.at("WaterFluxDiagonal")->texture->ClearTexture();
		masks.at("FlowAccumulationMask")->texture->ClearTexture();
		masks.at("RiverChannelMask")->texture->ClearTexture();
		if (m_terrainPassContext.cpuTerrainCacheValid)
		{
			std::fill(m_terrainPassContext.cpuWater.begin(), m_terrainPassContext.cpuWater.end(), 0.0f);
			std::fill(m_terrainPassContext.cpuFlowAccumulation.begin(), m_terrainPassContext.cpuFlowAccumulation.end(), 0.0f);
			std::fill(m_terrainPassContext.cpuRiverChannels.begin(), m_terrainPassContext.cpuRiverChannels.end(), 0.0f);
		}
		m_terrainMasks = masks;
	}

	void TerrainComputePipeline::ResetErosionSimulation(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		m_terrainPassContext.deferParticleTextureUpload = false;
		masks.at("FinalHeight")->texture->CopyFrom(*masks.at("BaseHeight")->texture);
		masks["ErodedHeight"] = masks.at("FinalHeight");
		m_terrainPassContext.cpuTerrainCacheValid = false;
		m_terrainPassContext.cpuHeight.clear();
		m_terrainPassContext.cpuBaseHeight.clear();
		m_terrainPassContext.cpuDeposit.clear();
		m_terrainPassContext.cpuWater.clear();
		m_terrainPassContext.cpuSediment.clear();
		m_terrainPassContext.cpuFlowAccumulation.clear();
		m_terrainPassContext.cpuRiverChannels.clear();
		m_terrainPassContext.cpuErosionDelta.clear();
		m_graph->ExecutePasses(TerrainPassTypes::MASK, masks, m_terrainPassContext);
		m_terrainMasks = masks;
	}

	void TerrainComputePipeline::BuildGraph()
	{
		auto elevationPass = std::make_shared<ElevationPass>(m_computeShaders["Elevation"], m_size);
		auto mountainMaskPass = std::make_shared<MountainMaskPass>(m_computeShaders["MountainMask"], m_size);
		auto mountainHeightPass = std::make_shared<MountainHeightPass>(m_computeShaders["MountainHeight"], m_size);
		auto finalMaskPass = std::make_shared<FinalMaskPass>(m_computeShaders["FinalMask"], m_size);
		auto finalHeightPass = std::make_shared<FinalHeightPass>(m_computeShaders["FinalHeight"], m_size);
		auto normalMaskPass = std::make_shared<NormalMaskPass>(m_computeShaders["NormalMask"], m_size);
		auto slopeMaskPass = std::make_shared<SlopeMaskPass>(m_computeShaders["SlopeMask"], m_size);
		auto particleErosionPass = std::make_shared<ParticleErosionPass>(m_size);
		particleErosionPass->SetHydraulicSettings(&m_terrainPassContext.hydraulicSettings);

		m_graph->AddPass(elevationPass);
		m_graph->AddPass(mountainMaskPass);
		m_graph->AddPass(mountainHeightPass);
		m_graph->AddPass(finalMaskPass);
		m_graph->AddPass(finalHeightPass);
		m_graph->AddPass(normalMaskPass);
		m_graph->AddPass(slopeMaskPass);
		m_graph->AddPass(particleErosionPass);

		PWG_INFO("Graph builded successfully");
	}

	void TerrainComputePipeline::AddComputeShader(std::string name, std::shared_ptr<ComputeShader> computeShader)
	{
		if (!m_computeShaders.contains(name))
		{
			m_computeShaders[name] = computeShader;
			PWG_INFO("Compute shader '{0}' added to a pipeline map", name);
		}
		else
		{
			PWG_ERROR("Compute shader '{0}' exists in a pipeline map", name);
		}
	}
	std::unordered_map<std::string, std::shared_ptr<TerrainMask>> TerrainComputePipeline::CreateMasks()
	{
		std::unordered_map<std::string, std::shared_ptr<TerrainMask>> terrainMasks;

		terrainMasks["Elevation"] = std::make_shared<TerrainMask>("Elevation", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::HEIGHT, 1.0f);
		terrainMasks["MountainMask"] = std::make_shared<TerrainMask>("MountainMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["MountainHeight"] = std::make_shared<TerrainMask>("MountainHeight", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::HEIGHT, 1.0f);
		terrainMasks["FinalMask"] = std::make_shared<TerrainMask>("FinalMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["FinalHeight"] = std::make_shared<TerrainMask>("FinalHeight", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["BaseHeight"] = std::make_shared<TerrainMask>("BaseHeight", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["NormalMask"] = std::make_shared<TerrainMask>("NormalMask", std::make_shared<Texture>(m_size, m_size, TextureType::NORMAL16F), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["SlopeMask"] = std::make_shared<TerrainMask>("SlopeMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["ErodedHeight"] = std::make_shared<TerrainMask>("ErodedHeight", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["WaterMask"] = std::make_shared<TerrainMask>("WaterMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["SedimentMask"] = std::make_shared<TerrainMask>("SedimentMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["DepositMask"] = std::make_shared<TerrainMask>("DepositMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["WaterFlux"] = std::make_shared<TerrainMask>("WaterFlux", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT4), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["WaterFluxDiagonal"] = std::make_shared<TerrainMask>("WaterFluxDiagonal", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT4), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["ErosionDelta"] = std::make_shared<TerrainMask>("ErosionDelta", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["FlowAccumulationMask"] = std::make_shared<TerrainMask>("FlowAccumulationMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["RiverChannelMask"] = std::make_shared<TerrainMask>("RiverChannelMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);

		for (const auto& mask : terrainMasks)
		{
			mask.second->texture->SetTextureWrapping(ToGL(TextureWrapMode::CLAMP_TO_EDGE), ToGL(TextureWrapMode::CLAMP_TO_EDGE));
		}

		m_terrainMasks = terrainMasks;

		return terrainMasks;
	}
	void TerrainComputePipeline::ClearTextures()
	{
		for (const auto& mask : m_terrainMasks)
		{
			mask.second->texture->ClearTexture();
		}
	}
	std::shared_ptr<TerrainGraph> TerrainComputePipeline::GetGraph()
	{
		return m_graph;
	}
	std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& TerrainComputePipeline::GetMasks()
	{
		return m_terrainMasks;
	}
	HydraulicSimulationSettings& TerrainComputePipeline::GetHydraulicSettings()
	{
		return m_terrainPassContext.hydraulicSettings;
	}
	int TerrainComputePipeline::GetSize()
	{
		return m_size;
	}
	std::shared_ptr<Texture> TerrainComputePipeline::GetFinalMaskTexture()
	{
		return m_terrainMasks["FinalHeight"]->texture;
	}
	std::unordered_map<std::string, std::shared_ptr<Texture>>& TerrainComputePipeline::GetFinalTerrainMasks()
	{
		m_finalTerrainMasks["FinalHeight"] = m_terrainMasks.at("FinalHeight")->texture;
		m_finalTerrainMasks["NormalMask"] = m_terrainMasks.at("NormalMask")->texture;

		return m_finalTerrainMasks;
	}
}


