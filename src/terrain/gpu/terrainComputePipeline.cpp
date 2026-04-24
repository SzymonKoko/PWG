#include "terrainComputePipeline.h"

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

		auto elevationShader = m_shaderManager->GetShader<pwg::ComputeShader>("elevationMaskShader");
		auto mountainMaskShader = m_shaderManager->GetShader<pwg::ComputeShader>("mountainMaskShader");
		auto mountainHeightShader = m_shaderManager->GetShader<pwg::ComputeShader>("mountainHeightShader");
		auto finalMaskShader = m_shaderManager->GetShader<pwg::ComputeShader>("finalMaskShader");
		auto finalHeightShader = m_shaderManager->GetShader<pwg::ComputeShader>("finalHeightShader");
		auto normalMaskShader = m_shaderManager->GetShader<pwg::ComputeShader>("normalMaskShader");
		auto slopeMaskShader = m_shaderManager->GetShader<pwg::ComputeShader>("slopeMaskShader");

		AddComputeShader("Elevation", elevationShader);
		AddComputeShader("MountainMask", mountainMaskShader);
		AddComputeShader("MountainHeight", mountainHeightShader);
		AddComputeShader("FinalMask", finalMaskShader);
		AddComputeShader("FinalHeight", finalHeightShader);
		AddComputeShader("NormalMask", normalMaskShader);
		AddComputeShader("SlopeMask", slopeMaskShader);
	}

	TerrainComputePipeline::~TerrainComputePipeline()
	{
	}

	void TerrainComputePipeline::Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{

		//m_terrainDebug->Clear();

		m_terrainPassContext.offset = glm::vec2(0,0);

		m_graph->Execute(masks, m_terrainPassContext);

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

		m_graph->AddPass(elevationPass);
		m_graph->AddPass(mountainMaskPass);
		m_graph->AddPass(mountainHeightPass);
		m_graph->AddPass(finalMaskPass);
		m_graph->AddPass(finalHeightPass);
		m_graph->AddPass(normalMaskPass);
		m_graph->AddPass(slopeMaskPass);

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

		terrainMasks["Elevation"] = std::make_shared<TerrainMask>("Elevation", std::make_shared<Texture>(m_size, m_size, ToGL(TextureFormats::R32F)), MaskUsage::HEIGHT, 1.0f);
		terrainMasks["MountainMask"] = std::make_shared<TerrainMask>("MountainMask", std::make_shared<Texture>(m_size, m_size, ToGL(TextureFormats::R32F)), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["MountainHeight"] = std::make_shared<TerrainMask>("MountainHeight", std::make_shared<Texture>(m_size, m_size, ToGL(TextureFormats::R32F)), MaskUsage::HEIGHT, 1.0f);
		terrainMasks["FinalMask"] = std::make_shared<TerrainMask>("FinalMask", std::make_shared<Texture>(m_size, m_size, ToGL(TextureFormats::R32F)), MaskUsage::HEIGHT, 1.0f);
		terrainMasks["FinalHeight"] = std::make_shared<TerrainMask>("FinalHeight", std::make_shared<Texture>(m_size, m_size, ToGL(TextureFormats::R32F)), MaskUsage::HEIGHT, 1.0f);
		terrainMasks["NormalMask"] = std::make_shared<TerrainMask>("NormalMask", std::make_shared<Texture>(m_size, m_size, ToGL(TextureFormats::RGBA16F)), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["SlopeMask"] = std::make_shared<TerrainMask>("SlopeMask", std::make_shared<Texture>(m_size, m_size, ToGL(TextureFormats::R32F)), MaskUsage::WEIGHTS, 1.0f);

		for (const auto& mask : terrainMasks)
		{
			mask.second->texture->SetTextureWrapping(ToGL(TextureWrapMode::CLAMP_TO_EDGE), ToGL(TextureWrapMode::CLAMP_TO_EDGE));
		}

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


