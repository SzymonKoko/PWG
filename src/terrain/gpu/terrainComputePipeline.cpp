#include "terrainComputePipeline.h"
#include <algorithm>
#include <glad/glad.h>
#include <unordered_map>

namespace pwg
{
	namespace
	{
		const TerrainGraphParameter* FindGraphParameter(const TerrainGraphNodeInstance& node, const std::string& id)
		{
			auto it = std::find_if(node.parameters.begin(), node.parameters.end(), [&](const TerrainGraphParameter& param)
			{
				return param.id == id;
			});
			return it != node.parameters.end() ? &(*it) : nullptr;
		}

		float GetFloatParameter(const TerrainGraphNodeInstance& node, const std::string& id, float fallback)
		{
			const auto* param = FindGraphParameter(node, id);
			return param ? param->floatValue : fallback;
		}

		int GetIntParameter(const TerrainGraphNodeInstance& node, const std::string& id, int fallback)
		{
			const auto* param = FindGraphParameter(node, id);
			return param ? param->intValue : fallback;
		}

		bool GetBoolParameter(const TerrainGraphNodeInstance& node, const std::string& id, bool fallback)
		{
			const auto* param = FindGraphParameter(node, id);
			return param ? param->boolValue : fallback;
		}

		std::string GetStringParameter(const TerrainGraphNodeInstance& node, const std::string& id, const std::string& fallback)
		{
			const auto* param = FindGraphParameter(node, id);
			return param ? param->stringValue : fallback;
		}

		std::string OutputKey(int nodeId, const std::string& portId)
		{
			return std::to_string(nodeId) + ":" + portId;
		}

		std::string TempMaskName(int nodeId, const std::string& portId)
		{
			return "node_" + std::to_string(nodeId) + "_" + portId;
		}

		std::shared_ptr<TerrainMask> EnsureFloatMask(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, const std::string& name, int size)
		{
			auto it = masks.find(name);
			if (it != masks.end() && it->second && it->second->texture)
			{
				return it->second;
			}
			auto mask = std::make_shared<TerrainMask>(name, std::make_shared<Texture>(size, size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
			mask->texture->SetTextureWrapping(ToGL(TextureWrapMode::CLAMP_TO_EDGE), ToGL(TextureWrapMode::CLAMP_TO_EDGE));
			glBindTexture(GL_TEXTURE_2D, mask->texture->GetTextureID());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
			glBindTexture(GL_TEXTURE_2D, 0);
			masks[name] = mask;
			return mask;
		}
	}

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
		m_shaderManager->LoadCompute("universalConstantShader", terrainShadersPath + "masks/universalConstant.comp");
		m_shaderManager->LoadCompute("universalNoiseHeightShader", terrainShadersPath + "masks/universalNoiseHeight.comp");
		m_shaderManager->LoadCompute("universalBinaryOpShader", terrainShadersPath + "masks/universalBinaryOp.comp");
		m_shaderManager->LoadCompute("universalLerpShader", terrainShadersPath + "masks/universalLerp.comp");
		m_shaderManager->LoadCompute("universalUnaryOpShader", terrainShadersPath + "masks/universalUnaryOp.comp");
		m_shaderManager->LoadCompute("universalMaskFilterShader", terrainShadersPath + "masks/universalMaskFilter.comp");
		m_shaderManager->LoadCompute("universalGraphPreviewShader", terrainShadersPath + "masks/universalGraphPreview.comp");

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
		auto universalConstantShader = m_shaderManager->GetShader<pwg::ComputeShader>("universalConstantShader");
		auto universalNoiseHeightShader = m_shaderManager->GetShader<pwg::ComputeShader>("universalNoiseHeightShader");
		auto universalBinaryOpShader = m_shaderManager->GetShader<pwg::ComputeShader>("universalBinaryOpShader");
		auto universalLerpShader = m_shaderManager->GetShader<pwg::ComputeShader>("universalLerpShader");
		auto universalUnaryOpShader = m_shaderManager->GetShader<pwg::ComputeShader>("universalUnaryOpShader");
		auto universalMaskFilterShader = m_shaderManager->GetShader<pwg::ComputeShader>("universalMaskFilterShader");
		auto universalGraphPreviewShader = m_shaderManager->GetShader<pwg::ComputeShader>("universalGraphPreviewShader");

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
		AddComputeShader("UniversalConstant", universalConstantShader);
		AddComputeShader("UniversalNoiseHeight", universalNoiseHeightShader);
		AddComputeShader("UniversalBinaryOp", universalBinaryOpShader);
		AddComputeShader("UniversalLerp", universalLerpShader);
		AddComputeShader("UniversalUnaryOp", universalUnaryOpShader);
		AddComputeShader("UniversalMaskFilter", universalMaskFilterShader);
		AddComputeShader("UniversalGraphPreview", universalGraphPreviewShader);

		m_universalGraphAsset = BuildDefaultUniversalTerrainGraph();
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
		m_terrainPassContext.worldOrigin = glm::vec2(0, 0);
		m_terrainPassContext.worldSize = m_size;
		m_terrainPassContext.deferParticleTextureUpload = false;
		if (m_universalGraphAsset.useUniversalGraph)
		{
			ExecuteUniversalGraph(masks);
		}
		else
		{
			m_graph->Execute(masks, m_terrainPassContext);
		}
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

	bool TerrainComputePipeline::ExecuteUniversalGraph(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		auto validation = ValidateTerrainGraphAsset(m_universalGraphAsset);
		if (!validation.valid)
		{
			for (const auto& message : validation.messages)
			{
				PWG_WARN("TerrainGraphV2 validation: {0}", message);
			}
			return false;
		}

		std::unordered_map<std::string, std::shared_ptr<TerrainMask>> outputs;
		std::vector<VegetationLayerDescriptor> vegetationLayers;
		auto outputFor = [&](int nodeId, const std::string& portId) -> std::shared_ptr<TerrainMask>
		{
			auto it = outputs.find(OutputKey(nodeId, portId));
			return it != outputs.end() ? it->second : nullptr;
		};
		auto inputFor = [&](const TerrainGraphNodeInstance& node, const std::string& portId) -> std::shared_ptr<TerrainMask>
		{
			const auto* link = m_universalGraphAsset.FindInputLink(node.id, portId);
			return link ? outputFor(link->fromNodeId, link->fromPortId) : nullptr;
		};
		auto setOutput = [&](const TerrainGraphNodeInstance& node, const std::string& portId, std::shared_ptr<TerrainMask> mask)
		{
			outputs[OutputKey(node.id, portId)] = mask;
		};
		auto makeOutput = [&](const TerrainGraphNodeInstance& node, const std::string& portId)
		{
			return EnsureFloatMask(masks, TempMaskName(node.id, portId), m_size);
		};

		for (int nodeId : validation.executionOrder)
		{
			const auto* node = m_universalGraphAsset.FindNode(nodeId);
			if (!node || !node->enabled)
			{
				continue;
			}

			if (node->typeId == "constant")
			{
				auto output = makeOutput(*node, "value");
				auto shader = m_computeShaders["UniversalConstant"];
				shader->ActivateShader();
				shader->SetUniformInt("u_size", m_size);
				shader->SetUniformFloat("u_value", GetFloatParameter(*node, "value", 0.5f));
				shader->BindImage(0, output->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
				shader->DispatchForTexture(m_size, m_size);
				shader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
				setOutput(*node, "value", output);
			}
			else if (node->typeId == "noiseHeight" || node->typeId == "noiseMask")
			{
				const char* outputPort = node->typeId == "noiseMask" ? "mask" : "height";
				auto output = makeOutput(*node, outputPort);
				auto shader = m_computeShaders["UniversalNoiseHeight"];
				shader->ActivateShader();
				shader->SetUniformInt("u_noiseType", GetIntParameter(*node, "noiseType", 0));
				shader->SetUniformInt("u_fractalType", GetIntParameter(*node, "fractalType", 1));
				shader->SetUniformInt("u_octaves", GetIntParameter(*node, "octaves", 4));
				shader->SetUniformFloat("u_lacunarity", GetFloatParameter(*node, "lacunarity", 2.0f));
				shader->SetUniformFloat("u_gain", GetFloatParameter(*node, "gain", 0.5f));
				shader->SetUniformFloat("u_weightedStrength", GetFloatParameter(*node, "weightedStrength", 0.0f));
				shader->SetUniformFloat("u_pingPongStrength", GetFloatParameter(*node, "pingPongStrength", 2.0f));
				shader->SetUniformInt("u_seed", GetIntParameter(*node, "seed", 42));
				shader->SetUniformInt("u_size", m_size);
				shader->SetUniformFloat("u_frequency", GetFloatParameter(*node, "frequency", 1.0f));
				shader->SetUniformFloat("u_scale", GetFloatParameter(*node, "scale", 128.0f));
				shader->SetUniformVec2("u_offset", m_terrainPassContext.offset);
				shader->BindImage(0, output->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
				shader->DispatchForTexture(m_size, m_size);
				shader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
				setOutput(*node, outputPort, output);
			}
			else if (node->typeId == "add" || node->typeId == "subtract" || node->typeId == "multiply" || node->typeId == "divide" || node->typeId == "min" || node->typeId == "max")
			{
				auto a = inputFor(*node, "a");
				auto b = inputFor(*node, "b");
				if (!a || !b)
				{
					return false;
				}
				auto output = makeOutput(*node, "result");
				int operation = 0;
				if (node->typeId == "subtract") operation = 1;
				else if (node->typeId == "multiply") operation = 2;
				else if (node->typeId == "divide") operation = 3;
				else if (node->typeId == "min") operation = 4;
				else if (node->typeId == "max") operation = 5;
				auto shader = m_computeShaders["UniversalBinaryOp"];
				shader->ActivateShader();
				a->texture->Bind(0);
				b->texture->Bind(1);
				shader->SetUniformInt("u_a", 0);
				shader->SetUniformInt("u_b", 1);
				shader->SetUniformInt("u_size", m_size);
				shader->SetUniformInt("u_operation", operation);
				shader->BindImage(0, output->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
				shader->DispatchForTexture(m_size, m_size);
				shader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
				setOutput(*node, "result", output);
			}
			else if (node->typeId == "lerp")
			{
				auto a = inputFor(*node, "a");
				auto b = inputFor(*node, "b");
				auto t = inputFor(*node, "t");
				if (!a || !b)
				{
					return false;
				}
				auto output = makeOutput(*node, "result");
				auto shader = m_computeShaders["UniversalLerp"];
				shader->ActivateShader();
				a->texture->Bind(0);
				b->texture->Bind(1);
				if (t)
				{
					t->texture->Bind(2);
				}
				shader->SetUniformInt("u_a", 0);
				shader->SetUniformInt("u_b", 1);
				shader->SetUniformInt("u_t", 2);
				shader->SetUniformInt("u_size", m_size);
				shader->SetUniformFloat("u_defaultT", 0.5f);
				shader->SetUniformInt("u_useTextureT", t ? 1 : 0);
				shader->BindImage(0, output->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
				shader->DispatchForTexture(m_size, m_size);
				shader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
				setOutput(*node, "result", output);
			}
			else if (node->typeId == "clamp" || node->typeId == "normalize" || node->typeId == "power" || node->typeId == "thresholdMask" || node->typeId == "heightBandMask" ||
				node->typeId == "abs" || node->typeId == "invert" || node->typeId == "fitRange" || node->typeId == "smoothstep" ||
				node->typeId == "terrace" || node->typeId == "posterizeMask")
			{
				const char* inputPort = "value";
				const char* outputPort = "result";
				if (node->typeId == "thresholdMask" || node->typeId == "heightBandMask" || node->typeId == "terrace")
				{
					inputPort = "height";
					outputPort = node->typeId == "terrace" ? "height" : "mask";
				}
				else if (node->typeId == "posterizeMask")
				{
					inputPort = "mask";
					outputPort = "mask";
				}
				auto value = inputFor(*node, inputPort);
				if (!value)
				{
					return false;
				}
				auto output = makeOutput(*node, outputPort);
				int operation = 0;
				if (node->typeId == "normalize") operation = 1;
				else if (node->typeId == "power") operation = 2;
				else if (node->typeId == "thresholdMask" || node->typeId == "heightBandMask") operation = 3;
				else if (node->typeId == "abs") operation = 4;
				else if (node->typeId == "invert") operation = 5;
				else if (node->typeId == "fitRange") operation = 6;
				else if (node->typeId == "smoothstep") operation = 7;
				else if (node->typeId == "terrace") operation = 8;
				else if (node->typeId == "posterizeMask") operation = 9;
				auto shader = m_computeShaders["UniversalUnaryOp"];
				shader->ActivateShader();
				value->texture->Bind(0);
				shader->SetUniformInt("u_value", 0);
				shader->SetUniformInt("u_size", m_size);
				shader->SetUniformInt("u_operation", operation);
				float minValue = node->typeId == "fitRange" ? GetFloatParameter(*node, "inMin", 0.0f) : GetFloatParameter(*node, "min", 0.0f);
				float maxValue = node->typeId == "fitRange" ? GetFloatParameter(*node, "inMax", 1.0f) : GetFloatParameter(*node, "max", 1.0f);
				shader->SetUniformFloat("u_min", minValue);
				shader->SetUniformFloat("u_max", maxValue);
				shader->SetUniformFloat("u_power", GetFloatParameter(*node, "power", GetFloatParameter(*node, "pivot", 1.0f)));
				shader->SetUniformFloat("u_outMin", GetFloatParameter(*node, "outMin", 0.0f));
				shader->SetUniformFloat("u_outMax", GetFloatParameter(*node, "outMax", 1.0f));
				shader->SetUniformInt("u_steps", GetIntParameter(*node, "steps", 8));
				shader->BindImage(0, output->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
				shader->DispatchForTexture(m_size, m_size);
				shader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
				setOutput(*node, outputPort, output);
			}
			else if (node->typeId == "slopeMask" || node->typeId == "curvatureMask" || node->typeId == "blurMask")
			{
				const char* inputPort = node->typeId == "blurMask" ? "mask" : "height";
				auto value = inputFor(*node, inputPort);
				if (!value)
				{
					return false;
				}
				auto output = makeOutput(*node, "mask");
				int operation = 0;
				if (node->typeId == "curvatureMask") operation = 1;
				else if (node->typeId == "blurMask") operation = 2;
				auto shader = m_computeShaders["UniversalMaskFilter"];
				shader->ActivateShader();
				value->texture->Bind(0);
				shader->SetUniformInt("u_value", 0);
				shader->SetUniformInt("u_size", m_size);
				shader->SetUniformInt("u_operation", operation);
				shader->SetUniformInt("u_radius", GetIntParameter(*node, "radius", 2));
				shader->SetUniformInt("u_mode", GetIntParameter(*node, "mode", 0));
				shader->SetUniformFloat("u_scale", GetFloatParameter(*node, "scale", 0.05f));
				shader->SetUniformFloat("u_min", GetFloatParameter(*node, "min", 0.0f));
				shader->SetUniformFloat("u_max", GetFloatParameter(*node, "max", 1.0f));
				shader->SetUniformFloat("u_bias", GetFloatParameter(*node, "bias", 0.5f));
				shader->BindImage(0, output->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
				shader->DispatchForTexture(m_size, m_size);
				shader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
				setOutput(*node, "mask", output);
			}
			else if (node->typeId == "outputHeight")
			{
				auto source = inputFor(*node, "height");
				if (!source)
				{
					return false;
				}
				masks.at("FinalHeight")->texture->CopyFrom(*source->texture);
			}
			else if (node->typeId == "outputMask")
			{
				auto source = inputFor(*node, "mask");
				if (!source)
				{
					return false;
				}
				std::string targetName = GetStringParameter(*node, "target", "FinalMask");
				auto target = EnsureFloatMask(masks, targetName, m_size);
				target->texture->CopyFrom(*source->texture);
			}
			else if (node->typeId == "treeAssetA" || node->typeId == "treeAssetB" || node->typeId == "grassAsset" || node->typeId == "vegetationAsset")
			{
				VegetationLayerDescriptor layer;
				layer.nodeId = node->id;
				layer.assetId = node->typeId == "treeAssetA"
					? "builtin.tree_a"
					: (node->typeId == "treeAssetB"
						? "builtin.tree_b"
						: (node->typeId == "grassAsset" ? "builtin.grass" : GetStringParameter(*node, "assetId", "builtin.tree_a")));
				layer.seed = GetIntParameter(*node, "seed", 1337);
				layer.density = std::clamp(GetFloatParameter(*node, "density", 0.5f), 0.0f, 1.0f);
				layer.spacing = std::max(GetFloatParameter(*node, "spacing", 50.0f), 1.0f);
				layer.minScale = GetFloatParameter(*node, "minScale", 0.8f);
				layer.maxScale = GetFloatParameter(*node, "maxScale", 1.2f);
				if (layer.minScale > layer.maxScale)
				{
					std::swap(layer.minScale, layer.maxScale);
				}
				layer.maxSlopeDegrees = std::clamp(GetFloatParameter(*node, "maxSlope", 30.0f), 0.0f, 89.0f);
				layer.alignToSlope = GetBoolParameter(*node, "alignToSlope", false);
				layer.maxTiltDegrees = std::clamp(GetFloatParameter(*node, "maxTilt", 30.0f), 0.0f, 89.0f);
				layer.renderDistance = std::max(GetFloatParameter(*node, "renderDistance", 1000.0f), 1.0f);
				layer.densityMask = inputFor(*node, "density");
				vegetationLayers.push_back(std::move(layer));
			}
		}

		m_graph->ExecutePasses(TerrainPassTypes::MASK, masks, m_terrainPassContext);
		auto previewShader = m_computeShaders["UniversalGraphPreview"];
		constexpr int previewSize = 256;
		for (const auto& node : m_universalGraphAsset.nodes)
		{
			const auto* type = FindTerrainGraphNodeType(node.typeId);
			if (!node.enabled || !type) continue;
			for (const auto& output : type->outputs)
			{
				if (output.valueType != TerrainGraphValueType::FloatMask) continue;
				auto source = masks.find(TempMaskName(node.id, output.id));
				if (source == masks.end() || !source->second || !source->second->texture) continue;
				auto range = EstimateTerrainGraphOutputRange(m_universalGraphAsset, node.id, output.id);
				if (!range.valid) range = { 0.0f, 1.0f, true };
				if (std::abs(range.maximum - range.minimum) < 0.0001f)
				{
					range.minimum = std::min(0.0f, range.minimum);
					range.maximum = std::max(1.0f, range.maximum);
				}
				auto preview = EnsureFloatMask(masks, "preview_" + TempMaskName(node.id, output.id), previewSize);
				previewShader->ActivateShader();
				source->second->texture->Bind(0);
				previewShader->SetUniformInt("u_value", 0);
				previewShader->SetUniformInt("u_size", previewSize);
				previewShader->SetUniformFloat("u_minValue", range.minimum);
				previewShader->SetUniformFloat("u_maxValue", range.maximum);
				previewShader->BindImage(0, preview->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
				previewShader->DispatchForTexture(previewSize, previewSize);
				previewShader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
			}
		}
		m_terrainMasks = masks;
		m_vegetationLayers = std::move(vegetationLayers);
		++m_vegetationRevision;
		return true;
	}

	void TerrainComputePipeline::StepSimulation(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		ApplyUniversalGraphSimulationSettings();
		m_terrainPassContext.offset = glm::vec2(0, 0);
		m_terrainPassContext.deferParticleTextureUpload = false;
		for (int i = 0; i < m_terrainPassContext.hydraulicSettings.stepsPerUpdate; i++)
		{
			m_graph->ExecutePasses(TerrainPassTypes::HYDROLOGY, masks, m_terrainPassContext);
			if (m_terrainPassContext.hydraulicSettings.erosionEnabled)
			{
				m_graph->ExecutePasses(TerrainPassTypes::EROSION, masks, m_terrainPassContext);
			}
		}
		m_graph->ExecutePasses(TerrainPassTypes::MASK, masks, m_terrainPassContext);
		m_terrainMasks = masks;
	}

	void TerrainComputePipeline::StepErosionSimulation(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		StepSimulation(masks);
	}

	void TerrainComputePipeline::StepParticleBake(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, bool uploadPreview)
	{
		ApplyUniversalGraphSimulationSettings();
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
		ApplyUniversalGraphSimulationSettings();
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
		ApplyUniversalGraphSimulationSettings();
		m_terrainPassContext.offset = glm::vec2(0, 0);
		for (int i = 0; i < m_terrainPassContext.hydraulicSettings.stepsPerUpdate; i++)
		{
			m_graph->ExecutePasses(TerrainPassTypes::HYDROLOGY, masks, m_terrainPassContext);
		}
		m_terrainMasks = masks;
	}

	void TerrainComputePipeline::SettleWaterSimulation(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, int iterations)
	{
		ApplyUniversalGraphSimulationSettings();
		m_terrainPassContext.offset = glm::vec2(0, 0);
		m_terrainPassContext.deferParticleTextureUpload = false;

		// Settle existing water without adding new rain and without eroding, so the
		// remaining water just keeps flowing downhill until it pools in the lowest
		// points. We temporarily zero the rain so the simulation drains to rest.
		float savedRain = m_terrainPassContext.hydraulicSettings.rainRate;
		m_terrainPassContext.hydraulicSettings.rainRate = 0.0f;

		int steps = std::max(iterations, 1);
		for (int i = 0; i < steps; i++)
		{
			m_graph->ExecutePasses(TerrainPassTypes::HYDROLOGY, masks, m_terrainPassContext);
		}

		m_terrainPassContext.hydraulicSettings.rainRate = savedRain;
		m_terrainMasks = masks;
	}

	void TerrainComputePipeline::RefreshMaskPass(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		m_terrainPassContext.offset = glm::vec2(0, 0);
		m_terrainPassContext.deferParticleTextureUpload = false;
		m_graph->ExecutePasses(TerrainPassTypes::MASK, masks, m_terrainPassContext);
		m_terrainMasks = masks;
	}

	void TerrainComputePipeline::GenerateWorld(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		RegenerateTerrain(masks);
	}

	TerrainPassContext& TerrainComputePipeline::GetPassContext()
	{
		return m_terrainPassContext;
	}

	void TerrainComputePipeline::ApplyUniversalGraphSimulationSettings()
	{
		auto& settings = m_terrainPassContext.hydraulicSettings;
		for (const auto& node : m_universalGraphAsset.nodes)
		{
			if (!node.enabled)
			{
				continue;
			}

			if (node.typeId == "hydraulicSimulation")
			{
				settings.autoWaterSimulation = GetBoolParameter(node, "autoWaterSimulation", settings.autoWaterSimulation);
				settings.stepsPerUpdate = GetIntParameter(node, "stepsPerUpdate", settings.stepsPerUpdate);
				settings.waterStepsPerSecond = GetIntParameter(node, "waterStepsPerSecond", settings.waterStepsPerSecond);
				settings.waterSubsteps = GetIntParameter(node, "waterSubsteps", settings.waterSubsteps);
				settings.rainRate = GetFloatParameter(node, "rainRate", settings.rainRate);
				settings.rainElevationBias = GetFloatParameter(node, "rainElevationBias", settings.rainElevationBias);
				settings.rainMinHeight = GetFloatParameter(node, "rainMinHeight", settings.rainMinHeight);
				settings.rainMaxHeight = GetFloatParameter(node, "rainMaxHeight", settings.rainMaxHeight);
				settings.evaporation = GetFloatParameter(node, "evaporation", settings.evaporation);
				settings.infiltrationRate = GetFloatParameter(node, "infiltrationRate", settings.infiltrationRate);
				settings.boundaryDrain = GetFloatParameter(node, "boundaryDrain", settings.boundaryDrain);
				settings.maxWaterDepth = GetFloatParameter(node, "maxWaterDepth", settings.maxWaterDepth);
				settings.pipeFlow = GetFloatParameter(node, "pipeFlow", settings.pipeFlow);
				settings.pipeDamping = GetFloatParameter(node, "pipeDamping", settings.pipeDamping);
				settings.flowConcentration = GetFloatParameter(node, "flowConcentration", settings.flowConcentration);
				settings.useDiagonalFlow = GetBoolParameter(node, "useDiagonalFlow", settings.useDiagonalFlow);
			}
			else if (node.typeId == "streamErosion")
			{
				settings.erosionEnabled = GetBoolParameter(node, "erosionEnabled", settings.erosionEnabled);
				settings.sedimentCapacity = GetFloatParameter(node, "sedimentCapacity", settings.sedimentCapacity);
				settings.erosionStrength = GetFloatParameter(node, "erosionStrength", settings.erosionStrength);
				settings.depositionStrength = GetFloatParameter(node, "depositionStrength", settings.depositionStrength);
				settings.flatteningStrength = GetFloatParameter(node, "flatteningStrength", settings.flatteningStrength);
				settings.minWaterToErode = GetFloatParameter(node, "minWaterToErode", settings.minWaterToErode);
				settings.minFlowToErode = GetFloatParameter(node, "minFlowToErode", settings.minFlowToErode);
				settings.maxHeightChange = GetFloatParameter(node, "maxHeightChange", settings.maxHeightChange);
				settings.maxErosionDepth = GetFloatParameter(node, "maxErosionDepth", settings.maxErosionDepth);
				settings.maxDepositHeight = GetFloatParameter(node, "maxDepositHeight", settings.maxDepositHeight);
				settings.flowAccumulationGrowth = GetFloatParameter(node, "flowAccumulationGrowth", settings.flowAccumulationGrowth);
				settings.flowAccumulationDecay = GetFloatParameter(node, "flowAccumulationDecay", settings.flowAccumulationDecay);
				settings.riverChannelGrowth = GetFloatParameter(node, "riverChannelGrowth", settings.riverChannelGrowth);
				settings.riverChannelDecay = GetFloatParameter(node, "riverChannelDecay", settings.riverChannelDecay);
				settings.riverChannelInfluence = GetFloatParameter(node, "riverChannelInfluence", settings.riverChannelInfluence);
				settings.riverChannelDepth = GetFloatParameter(node, "riverChannelDepth", settings.riverChannelDepth);
			}
			else if (node.typeId == "particleErosion")
			{
				settings.particleErosionEnabled = GetBoolParameter(node, "particleErosionEnabled", settings.particleErosionEnabled);
				settings.bakeParticleSteps = GetIntParameter(node, "bakeParticleSteps", settings.bakeParticleSteps);
				settings.bakeStepsPerFrame = GetIntParameter(node, "bakeStepsPerFrame", settings.bakeStepsPerFrame);
				settings.bakeUploadInterval = GetIntParameter(node, "bakeUploadInterval", settings.bakeUploadInterval);
				settings.particlesPerStep = GetIntParameter(node, "particlesPerStep", settings.particlesPerStep);
				settings.dropletLifetime = GetIntParameter(node, "dropletLifetime", settings.dropletLifetime);
				settings.particleSeed = GetIntParameter(node, "particleSeed", settings.particleSeed);
				settings.particleInertia = GetFloatParameter(node, "particleInertia", settings.particleInertia);
				settings.particleGravity = GetFloatParameter(node, "particleGravity", settings.particleGravity);
				settings.particleInitialWater = GetFloatParameter(node, "particleInitialWater", settings.particleInitialWater);
				settings.particleEvaporation = GetFloatParameter(node, "particleEvaporation", settings.particleEvaporation);
				settings.particleMinSlope = GetFloatParameter(node, "particleMinSlope", settings.particleMinSlope);
				settings.particleCapacity = GetFloatParameter(node, "particleCapacity", settings.particleCapacity);
				settings.particleErosionStrength = GetFloatParameter(node, "particleErosionStrength", settings.particleErosionStrength);
				settings.particleDepositionStrength = GetFloatParameter(node, "particleDepositionStrength", settings.particleDepositionStrength);
				settings.particleBrushRadius = GetFloatParameter(node, "particleBrushRadius", settings.particleBrushRadius);
				settings.particleDepositRadius = GetFloatParameter(node, "particleDepositRadius", settings.particleDepositRadius);
				settings.particleChannelStrength = GetFloatParameter(node, "particleChannelStrength", settings.particleChannelStrength);
				settings.particleFlowStrength = GetFloatParameter(node, "particleFlowStrength", settings.particleFlowStrength);
			}
			else if (node.typeId == "lakeCleanup")
			{
				settings.lakeMinDepth = GetFloatParameter(node, "lakeMinDepth", settings.lakeMinDepth);
				settings.lakeLeveling = GetFloatParameter(node, "lakeLeveling", settings.lakeLeveling);
				settings.lakeFillEnabled = GetFloatParameter(node, "lakeFillEnabled", settings.lakeFillEnabled);
				settings.lakeMinFlow = GetFloatParameter(node, "lakeMinFlow", settings.lakeMinFlow);
				settings.lakeMaxDepth = GetFloatParameter(node, "lakeMaxDepth", settings.lakeMaxDepth);
				settings.lakeBedSmoothing = GetFloatParameter(node, "lakeBedSmoothing", settings.lakeBedSmoothing);
				settings.lakeWaterSmoothing = GetFloatParameter(node, "lakeWaterSmoothing", settings.lakeWaterSmoothing);
				settings.lakeInfiltrationScale = GetFloatParameter(node, "lakeInfiltrationScale", settings.lakeInfiltrationScale);
			}
		}
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
		auto hydraulicFluxPass = std::make_shared<HydraulicFluxPass>(m_computeShaders["HydraulicFlux"], m_size);
		auto hydraulicWaterPass = std::make_shared<HydraulicWaterPass>(m_computeShaders["HydraulicWater"], m_size);
		auto erosionPass = std::make_shared<ErosionPass>(m_computeShaders["ErosionMask"], m_size);
		hydraulicFluxPass->SetHydraulicSettings(&m_terrainPassContext.hydraulicSettings);
		hydraulicWaterPass->SetHydraulicSettings(&m_terrainPassContext.hydraulicSettings);
		erosionPass->SetHydraulicSettings(&m_terrainPassContext.hydraulicSettings);

		m_graph->AddPass(elevationPass);
		m_graph->AddPass(mountainMaskPass);
		m_graph->AddPass(mountainHeightPass);
		m_graph->AddPass(finalMaskPass);
		m_graph->AddPass(finalHeightPass);
		m_graph->AddPass(normalMaskPass);
		m_graph->AddPass(slopeMaskPass);
		m_graph->AddPass(hydraulicFluxPass);
		m_graph->AddPass(hydraulicWaterPass);
		m_graph->AddPass(erosionPass);

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
	TerrainGraphAsset& TerrainComputePipeline::GetUniversalGraphAsset()
	{
		return m_universalGraphAsset;
	}
	const TerrainGraphAsset& TerrainComputePipeline::GetUniversalGraphAsset() const
	{
		return m_universalGraphAsset;
	}
	const std::vector<VegetationLayerDescriptor>& TerrainComputePipeline::GetVegetationLayers() const
	{
		return m_vegetationLayers;
	}
	std::uint64_t TerrainComputePipeline::GetVegetationRevision() const
	{
		return m_vegetationRevision;
	}
	void TerrainComputePipeline::ResetUniversalGraphToDefault()
	{
		m_universalGraphAsset = BuildDefaultUniversalTerrainGraph();
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


