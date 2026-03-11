#include "terrainComputePipeline.h"

namespace pwg
{
	TerrainComputePipeline::TerrainComputePipeline(int terrainSize)
		: m_size(terrainSize)
	{
		m_graph = std::make_shared<TerrainGraph>();

	}

	TerrainComputePipeline::~TerrainComputePipeline()
	{
	}

	void TerrainComputePipeline::Execute()
	{
		ClearTextures();
		m_graph->Execute(m_terrainMasks);
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

		m_terrainMasks["Elevation"] = std::make_shared<TerrainMask>("Elevation", std::make_shared<Texture>(m_size, m_size, ToGL(TextureFormats::R32F)), MaskUsage::HEIGHT, 1.0f);
		m_terrainMasks["MountainMask"] = std::make_shared<TerrainMask>("MountainMask", std::make_shared<Texture>(m_size, m_size, ToGL(TextureFormats::R32F)), MaskUsage::WEIGHTS, 1.0f);		
		m_terrainMasks["MountainHeight"] = std::make_shared<TerrainMask>("MountainHeight", std::make_shared<Texture>(m_size, m_size, ToGL(TextureFormats::R32F)), MaskUsage::HEIGHT, 1.0f);		
		m_terrainMasks["FinalMask"] = std::make_shared<TerrainMask>("FinalMask", std::make_shared<Texture>(m_size, m_size, ToGL(TextureFormats::R32F)), MaskUsage::HEIGHT, 1.0f);
		m_terrainMasks["FinalHeight"] = std::make_shared<TerrainMask>("FinalHeight", std::make_shared<Texture>(m_size, m_size, ToGL(TextureFormats::R32F)), MaskUsage::HEIGHT, 1.0f);
		m_terrainMasks["NormalMask"] = std::make_shared<TerrainMask>("NormalMask", std::make_shared<Texture>(m_size, m_size, ToGL(TextureFormats::RGBA16F)), MaskUsage::WEIGHTS, 1.0f);
		m_terrainMasks["SlopeMask"] = std::make_shared<TerrainMask>("SlopeMask", std::make_shared<Texture>(m_size, m_size, ToGL(TextureFormats::R32F)), MaskUsage::WEIGHTS, 1.0f);
		

		for (const auto& mask : m_terrainMasks)
		{
			mask.second->texture->SetTextureWrapping(ToGL(TextureWrapMode::CLAMP_TO_EDGE), ToGL(TextureWrapMode::CLAMP_TO_EDGE));
		}

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


