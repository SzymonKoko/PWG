#include "terrainComputePipeline.h"
#include "debug/debugView.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <glad/glad.h>
#include <limits>
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

		/** @brief Hands the worker's stage times of a finished hydrology run to the Debug View (main thread). */
		void PublishHydrologyTiming(const DrainageNetworkResult& result)
		{
			if constexpr (debug::InstrumentationCompiled)
			{
				if (!debug::DebugView::IsEnabled()) return;
				auto& stats = debug::DebugView::Get().GetSimulationStats();
				if (!result.IsValid())
				{
					stats.AbortHydrologyRun();
					return;
				}
				std::array<const char*, DrainageNetworkResult::StageCount> names{};
				for (size_t i = 0; i < names.size(); ++i)
				{
					names[i] = DrainageNetworkGenerator::GetStageName(static_cast<int>(i) + 1);
				}
				stats.SubmitHydrologyResult(result.stageMs.data(), result.stageMs.size(), names.data(),
					result.resampleMs, result.workerMs);
			}
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
		m_shaderManager->LoadCompute("buildWaterRenderCacheShader", terrainShadersPath + "water/buildWaterRenderCache.comp");
		m_shaderManager->LoadCompute("downsampleWaterRenderCacheShader", terrainShadersPath + "water/downsampleWaterRenderCache.comp");
		m_shaderManager->LoadCompute("particleHydrologyShader", terrainShadersPath + "erosion/particleHydrology.comp");
		m_shaderManager->LoadCompute("applyParticleHydrologyShader", terrainShadersPath + "erosion/applyParticleHydrology.comp");
		m_shaderManager->LoadCompute("talusRelaxShader", terrainShadersPath + "erosion/talusRelax.comp");
		m_shaderManager->LoadCompute("thermalWeatheringShader", terrainShadersPath + "erosion/thermalWeathering.comp");
		m_shaderManager->LoadCompute("thermalTransportShader", terrainShadersPath + "erosion/thermalTransport.comp");
		m_shaderManager->LoadCompute("applyThermalErosionShader", terrainShadersPath + "erosion/applyThermalErosion.comp");
		m_shaderManager->Load("particleHydrologyDebugShader", terrainShadersPath + "erosion/particleDebug.vert", terrainShadersPath + "erosion/particleDebug.frag");
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
		auto particleHydrologyShader = m_shaderManager->GetShader<pwg::ComputeShader>("particleHydrologyShader");
		auto applyParticleHydrologyShader = m_shaderManager->GetShader<pwg::ComputeShader>("applyParticleHydrologyShader");
		auto talusRelaxShader = m_shaderManager->GetShader<pwg::ComputeShader>("talusRelaxShader");
		auto thermalWeatheringShader = m_shaderManager->GetShader<pwg::ComputeShader>("thermalWeatheringShader");
		auto thermalTransportShader = m_shaderManager->GetShader<pwg::ComputeShader>("thermalTransportShader");
		auto applyThermalErosionShader = m_shaderManager->GetShader<pwg::ComputeShader>("applyThermalErosionShader");
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
		AddComputeShader("ParticleHydrology", particleHydrologyShader);
		AddComputeShader("ApplyParticleHydrology", applyParticleHydrologyShader);
		AddComputeShader("TalusRelax", talusRelaxShader);
		AddComputeShader("ThermalWeathering", thermalWeatheringShader);
		AddComputeShader("ThermalTransport", thermalTransportShader);
		AddComputeShader("ApplyThermalErosion", applyThermalErosionShader);
		AddComputeShader("UniversalConstant", universalConstantShader);
		AddComputeShader("UniversalNoiseHeight", universalNoiseHeightShader);
		AddComputeShader("UniversalBinaryOp", universalBinaryOpShader);
		AddComputeShader("UniversalLerp", universalLerpShader);
		AddComputeShader("UniversalUnaryOp", universalUnaryOpShader);
		AddComputeShader("UniversalMaskFilter", universalMaskFilterShader);
		AddComputeShader("UniversalGraphPreview", universalGraphPreviewShader);

		m_universalGraphAsset = BuildDefaultUniversalTerrainGraph();
		m_gpuParticleErosion = std::make_unique<GpuParticleErosion>(
			m_size,
			particleHydrologyShader,
			applyParticleHydrologyShader,
			talusRelaxShader,
			m_shaderManager->GetShader<pwg::Shader>("particleHydrologyDebugShader"));
		m_gpuThermalErosion = std::make_unique<GpuThermalErosion>(
			m_size, thermalWeatheringShader, thermalTransportShader, applyThermalErosionShader);
		m_waterClipmapRenderer = std::make_unique<WaterClipmapRenderer>(
			m_size,
			m_shaderManager->GetShader<pwg::ComputeShader>("buildWaterRenderCacheShader"),
			m_shaderManager->GetShader<pwg::ComputeShader>("downsampleWaterRenderCacheShader"));
		m_drainageNetworkGenerator = std::make_unique<DrainageNetworkGenerator>();
	}

	TerrainComputePipeline::~TerrainComputePipeline()
	{
		CancelHydrology();
		PauseParticleErosion();
		PauseThermalErosion();
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
		InvalidateWaterRenderCache();
	}

	bool TerrainComputePipeline::ExecuteUniversalGraph(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		// TIMESTAMP span over the whole call; the nodes inside use TIME_ELAPSED (design 5.6).
		PWG_SIM_EVENT_GPU(graphEvent, debug::SimEventType::GRAPH_EXECUTE, debug::GpuEventMode::SPAN);
		PWG_SIM_EVENT_SET(graphEvent, resolution, m_size);
		PWG_SIM_EVENT_SET(graphEvent, success, false);
		PauseParticleErosion();
		PauseThermalErosion();
		m_particleErosionReady = false;
		m_thermalErosionReady = false;
		m_thermalErosionInitialized = false;
		ApplyUniversalGraphSimulationSettings();
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
			if (!node)
			{
				continue;
			}
			PWG_SIM_EVENT_GPU(nodeEvent, debug::SimEventType::GRAPH_NODE, debug::GpuEventMode::ELAPSED);
			PWG_SIM_EVENT_DO(nodeEvent,
				nodeEvent.Event().nodeId = node->id;
				nodeEvent.Event().nodeType = node->typeId;
				nodeEvent.Event().nodeEnabled = node->enabled;
				nodeEvent.Event().resolution = m_size);
			if (!node->enabled)
			{
				if (node->typeId == "particleErosion" || node->typeId == "thermalErosion")
				{
					auto source = inputFor(*node, "height");
					if (!source) return false;
					setOutput(*node, "height", source);
					const std::vector<const char*> ports = node->typeId == "particleErosion"
						? std::vector<const char*>{ "flow", "deposition", "erosion" }
						: std::vector<const char*>{ "debris", "deposition", "erosion" };
					for (const char* port : ports)
					{
						auto output = makeOutput(*node, port);
						output->texture->ClearTexture();
						setOutput(*node, port, output);
					}
					if (node->typeId == "particleErosion")
					{
						masks.at("DepositMask")->texture->ClearTexture();
						masks.at("FlowAccumulationMask")->texture->ClearTexture();
						masks.at("RiverChannelMask")->texture->ClearTexture();
						masks.at("ErosionDelta")->texture->ClearTexture();
					}
					else
					{
						masks.at("ThermalDebris")->texture->ClearTexture();
						masks.at("ThermalDebrisMask")->texture->ClearTexture();
						masks.at("ThermalErosionMask")->texture->ClearTexture();
						masks.at("ThermalDepositionMask")->texture->ClearTexture();
					}
				}
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
			else if (node->typeId == "slopeMask" || node->typeId == "curvatureMask" || node->typeId == "blurMask" ||
				node->typeId == "erodeMask" || node->typeId == "dilateMask")
			{
				const bool usesMaskInput = node->typeId == "blurMask" || node->typeId == "erodeMask" || node->typeId == "dilateMask";
				const char* inputPort = usesMaskInput ? "mask" : "height";
				auto value = inputFor(*node, inputPort);
				if (!value)
				{
					return false;
				}
				auto output = makeOutput(*node, "mask");
				int operation = 0;
				if (node->typeId == "curvatureMask") operation = 1;
				else if (node->typeId == "blurMask") operation = 2;
				else if (node->typeId == "erodeMask") operation = 3;
				else if (node->typeId == "dilateMask") operation = 4;
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
			else if (node->typeId == "particleErosion")
			{
				auto source = inputFor(*node, "height");
				if (!source) return false;
				float amount = std::clamp(GetFloatParameter(*node, "amount", 0.35f), 0.0f, 1.0f);
				int seed = GetIntParameter(*node, "seed", 1337);
				setOutput(*node, "height", source);
				setOutput(*node, "flow", masks.at("ErosionFlowMask"));
				setOutput(*node, "deposition", masks.at("DepositMask"));
				setOutput(*node, "erosion", masks.at("ErosionMask"));
				masks[TempMaskName(node->id, "flow")] = masks.at("ErosionFlowMask");
				masks[TempMaskName(node->id, "deposition")] = masks.at("DepositMask");
				masks[TempMaskName(node->id, "erosion")] = masks.at("ErosionMask");
				m_gpuParticleErosion->Reset(seed, amount);
				m_particleErosionReady = amount > 0.0001f;
				m_particleStepCount = 0;
				m_particleStepAccumulator = 0.0f;
			}
			else if (node->typeId == "thermalErosion")
			{
				auto source = inputFor(*node, "height");
				if (!source) return false;
				m_thermalAmount = std::clamp(GetFloatParameter(*node, "amount", 0.35f), 0.0f, 1.0f);
				m_thermalTalusAngle = std::clamp(GetFloatParameter(*node, "talusAngle", 32.0f), 15.0f, 60.0f);
				m_thermalWeathering = std::clamp(GetFloatParameter(*node, "weathering", 0.45f), 0.0f, 1.0f);
				auto hardness = inputFor(*node, "hardness");
				m_thermalHardness = hardness ? hardness->texture : nullptr;
				setOutput(*node, "height", source);
				setOutput(*node, "debris", masks.at("ThermalDebrisMask"));
				setOutput(*node, "erosion", masks.at("ThermalErosionMask"));
				setOutput(*node, "deposition", masks.at("ThermalDepositionMask"));
				masks[TempMaskName(node->id, "debris")] = masks.at("ThermalDebrisMask");
				masks[TempMaskName(node->id, "erosion")] = masks.at("ThermalErosionMask");
				masks[TempMaskName(node->id, "deposition")] = masks.at("ThermalDepositionMask");
				m_gpuThermalErosion->Reset(m_thermalAmount, m_thermalTalusAngle, m_thermalWeathering);
				m_thermalErosionReady = m_thermalAmount > 0.0001f;
				m_thermalStepCount = 0;
				m_thermalStepAccumulator = 0.0f;
			}
			else if (node->typeId == "drainageNetwork")
			{
				setOutput(*node, "water", masks.at("WaterMask"));
				setOutput(*node, "flow", masks.at("FlowAccumulationMask"));
				setOutput(*node, "rivers", masks.at("RiverChannelMask"));
				setOutput(*node, "lakes", masks.at("LakeMask"));
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
			// Asset nodes are compiled separately so spawning cannot reset erosion or water.
		}

		PWG_SIM_EVENT_GPU(maskPassEvent, debug::SimEventType::GRAPH_MASK_PASS, debug::GpuEventMode::ELAPSED);
		PWG_SIM_EVENT_SET(maskPassEvent, resolution, m_size);
		masks.at("WaterMask")->texture->ClearTexture();
		masks.at("SedimentMask")->texture->ClearTexture();
		masks.at("FlowAccumulationMask")->texture->ClearTexture();
		masks.at("RiverChannelMask")->texture->ClearTexture();
		masks.at("DepositMask")->texture->ClearTexture();
		masks.at("ErosionMask")->texture->ClearTexture();
		masks.at("ErosionDelta")->texture->ClearTexture();
		masks.at("ErosionWaterMask")->texture->ClearTexture();
		masks.at("ErosionFlowMask")->texture->ClearTexture();
		masks.at("ErosionChannelMask")->texture->ClearTexture();
		masks.at("LakeMask")->texture->ClearTexture();
		masks.at("WaterVelocityMap")->texture->ClearTexture();
		masks.at("RiverCarveDelta")->texture->ClearTexture();
		masks.at("ThermalBaseHeight")->texture->ClearTexture();
		masks.at("ThermalDebris")->texture->ClearTexture();
		masks.at("ThermalDebrisMask")->texture->ClearTexture();
		masks.at("ThermalErosionMask")->texture->ClearTexture();
		masks.at("ThermalDepositionMask")->texture->ClearTexture();
		m_hydrologyGenerated = false;
		m_hydrologyStatus = "Hydrology ready to generate";
		m_graph->ExecutePasses(TerrainPassTypes::MASK, masks, m_terrainPassContext);
		PWG_SIM_EVENT_END(maskPassEvent);
		PWG_SIM_EVENT_GPU(previewEvent, debug::SimEventType::GRAPH_PREVIEWS, debug::GpuEventMode::ELAPSED);
		for (const auto& node : m_universalGraphAsset.nodes)
		{
			if (!node.enabled || node.typeId != "particleErosion") continue;
			masks[TempMaskName(node.id, "height")] = masks.at("FinalHeight");
			masks[TempMaskName(node.id, "flow")] = masks.at("ErosionFlowMask");
			masks[TempMaskName(node.id, "deposition")] = masks.at("DepositMask");
			masks[TempMaskName(node.id, "erosion")] = masks.at("ErosionMask");
		}
		for (const auto& node : m_universalGraphAsset.nodes)
		{
			if (!node.enabled || node.typeId != "thermalErosion") continue;
			masks[TempMaskName(node.id, "height")] = masks.at("FinalHeight");
			masks[TempMaskName(node.id, "debris")] = masks.at("ThermalDebrisMask");
			masks[TempMaskName(node.id, "erosion")] = masks.at("ThermalErosionMask");
			masks[TempMaskName(node.id, "deposition")] = masks.at("ThermalDepositionMask");
		}
		for (const auto& node : m_universalGraphAsset.nodes)
		{
			if (!node.enabled || node.typeId != "drainageNetwork") continue;
			masks[TempMaskName(node.id, "water")] = masks.at("WaterMask");
			masks[TempMaskName(node.id, "flow")] = masks.at("FlowAccumulationMask");
			masks[TempMaskName(node.id, "rivers")] = masks.at("RiverChannelMask");
			masks[TempMaskName(node.id, "lakes")] = masks.at("LakeMask");

			auto previewShader = m_computeShaders["UniversalGraphPreview"];
			for (const auto& [port, source] : std::array<std::pair<const char*, const char*>, 4>{ {
				{ "water", "WaterMask" }, { "flow", "FlowAccumulationMask" },
				{ "rivers", "RiverChannelMask" }, { "lakes", "LakeMask" } } })
			{
				auto preview = EnsureFloatMask(masks, "preview_" + TempMaskName(node.id, port), 256);
				previewShader->ActivateShader();
				masks.at(source)->texture->Bind(0);
				previewShader->SetUniformInt("u_value", 0);
				previewShader->SetUniformInt("u_size", 256);
				previewShader->SetUniformFloat("u_minValue", 0.0f);
				previewShader->SetUniformFloat("u_maxValue", 1.0f);
				previewShader->BindImage(0, preview->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
				previewShader->DispatchForTexture(256, 256);
				previewShader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
			}
		}
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
		PWG_SIM_EVENT_END(previewEvent);
		m_terrainMasks = masks;
		InvalidateWaterRenderCache();
		PWG_SIM_EVENT_SET(graphEvent, success, true);
		return true;
	}

	bool TerrainComputePipeline::RefreshProceduralAssets(const std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		std::vector<VegetationLayerDescriptor> layers;
		for (const auto& node : m_universalGraphAsset.nodes)
		{
			if (!node.enabled || (node.typeId != "treeAssetA" && node.typeId != "treeAssetB" &&
				node.typeId != "grassAsset" && node.typeId != "vegetationAsset")) continue;

			VegetationLayerDescriptor layer;
			layer.nodeId = node.id;
			layer.assetId = node.typeId == "treeAssetA" ? "builtin.tree_a"
				: (node.typeId == "treeAssetB" ? "builtin.tree_b"
					: (node.typeId == "grassAsset" ? "builtin.grass" : GetStringParameter(node, "assetId", "builtin.tree_a")));
			layer.seed = GetIntParameter(node, "seed", 1337);
			layer.density = std::clamp(GetFloatParameter(node, "density", 0.5f), 0.0f, 1.0f);
			layer.spacing = std::max(GetFloatParameter(node, "spacing", 50.0f), 1.0f);
			layer.minScale = GetFloatParameter(node, "minScale", 0.8f);
			layer.maxScale = GetFloatParameter(node, "maxScale", 1.2f);
			if (layer.minScale > layer.maxScale) std::swap(layer.minScale, layer.maxScale);
			layer.maxSlopeDegrees = std::clamp(GetFloatParameter(node, "maxSlope", 30.0f), 0.0f, 89.0f);
			layer.alignToSlope = GetBoolParameter(node, "alignToSlope", false);
			layer.maxTiltDegrees = std::clamp(GetFloatParameter(node, "maxTilt", 30.0f), 0.0f, 89.0f);
			layer.renderDistance = std::max(GetFloatParameter(node, "renderDistance", 1000.0f), 1.0f);

			if (const auto* link = m_universalGraphAsset.FindInputLink(node.id, "density"))
			{
				auto found = masks.find(TempMaskName(link->fromNodeId, link->fromPortId));
				if (found == masks.end() || !found->second || !found->second->texture)
				{
					m_proceduralAssetStatus = "Run Graph first to calculate connected masks";
					return false;
				}
				layer.densityMask = found->second;
			}
			layers.push_back(std::move(layer));
		}

		m_vegetationLayers = std::move(layers);
		++m_vegetationRevision;
		m_proceduralAssetStatus = "Spawned " + std::to_string(m_vegetationLayers.size()) + " asset layers";
		return true;
	}

	const std::string& TerrainComputePipeline::GetProceduralAssetStatus() const { return m_proceduralAssetStatus; }

	bool TerrainComputePipeline::StepParticleErosion(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		if (!m_particleErosionReady || !m_gpuParticleErosion) return false;
		PWG_SIM_EVENT(stepEvent, debug::SimEventType::HYDRAULIC_STEP);
		PWG_SIM_EVENT_GPU_BEGIN(stepEvent, debug::GpuEventMode::ELAPSED);
		m_gpuParticleErosion->Step(
			masks.at("FinalHeight")->texture,
			masks.at("ErosionWaterMask")->texture,
			masks.at("ErosionFlowMask")->texture,
			masks.at("ErosionChannelMask")->texture,
			masks.at("DepositMask")->texture,
			masks.at("ErosionMask")->texture,
			masks.at("ErosionDelta")->texture);
		PWG_SIM_EVENT_GPU_END(stepEvent);
		++m_particleStepCount;
		PWG_SIM_EVENT_DO(stepEvent,
			stepEvent.Event().stepIndex = m_particleStepCount;
			stepEvent.Event().solverStep = m_gpuParticleErosion->GetStepCount();
			stepEvent.Event().talusStep = m_gpuParticleErosion->RanTalusLastStep();
			stepEvent.Event().activeParticles = m_gpuParticleErosion->GetParticleCount();
			stepEvent.Event().refreshStep = (m_particleStepCount & 3u) == 0u;
			stepEvent.Event().resolution = m_size);
		if ((m_particleStepCount & 3u) == 0u)
		{
			PWG_SIM_EVENT_GPU(refreshEvent, debug::SimEventType::HYDRAULIC_REFRESH, debug::GpuEventMode::ELAPSED);
			PWG_SIM_EVENT_SET(refreshEvent, stepIndex, m_particleStepCount);
			RefreshMaskPass(masks);
			RefreshParticleErosionPreviews(masks);
		}
		m_terrainMasks = masks;
		return true;
	}

	bool TerrainComputePipeline::ResetParticleErosionState(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		PauseParticleErosion();
		masks.at("FinalHeight")->texture->CopyFrom(*masks.at("BaseHeight")->texture);
		for (const char* name : { "ErosionWaterMask", "ErosionFlowMask", "ErosionChannelMask",
			"DepositMask", "ErosionMask", "ErosionDelta" })
		{
			masks.at(name)->texture->ClearTexture();
		}

		float amount = 0.0f;
		int seed = 1337;
		for (const auto& node : m_universalGraphAsset.nodes)
		{
			if (!node.enabled || node.typeId != "particleErosion") continue;
			amount = std::clamp(GetFloatParameter(node, "amount", 0.35f), 0.0f, 1.0f);
			seed = GetIntParameter(node, "seed", 1337);
			masks[TempMaskName(node.id, "height")] = masks.at("FinalHeight");
			masks[TempMaskName(node.id, "flow")] = masks.at("ErosionFlowMask");
			masks[TempMaskName(node.id, "deposition")] = masks.at("DepositMask");
			masks[TempMaskName(node.id, "erosion")] = masks.at("ErosionMask");
			break;
		}
		m_gpuParticleErosion->Reset(seed, amount);
		m_particleErosionReady = amount > 0.0001f;
		m_particleStepCount = 0;
		RefreshMaskPass(masks);
		RefreshParticleErosionPreviews(masks);
		m_terrainMasks = masks;
		InvalidateWaterRenderCache();
		return true;
	}

	void TerrainComputePipeline::RefreshParticleErosionPreviews(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		auto shader = m_computeShaders.at("UniversalGraphPreview");
		constexpr int previewSize = 256;
		for (const auto& node : m_universalGraphAsset.nodes)
		{
			if (!node.enabled || node.typeId != "particleErosion") continue;
			for (const char* port : { "height", "flow", "deposition", "erosion" })
			{
				auto source = masks.find(TempMaskName(node.id, port));
				if (source == masks.end() || !source->second || !source->second->texture) continue;
				auto range = EstimateTerrainGraphOutputRange(m_universalGraphAsset, node.id, port);
				if (!range.valid || std::abs(range.maximum - range.minimum) < 0.0001f)
				{
					range = { 0.0f, 1.0f, true };
				}
				auto preview = EnsureFloatMask(masks, "preview_" + TempMaskName(node.id, port), previewSize);
				shader->ActivateShader();
				shader->SetUniformInt("u_value", 0);
				shader->SetUniformInt("u_size", previewSize);
				shader->SetUniformFloat("u_minValue", range.minimum);
				shader->SetUniformFloat("u_maxValue", range.maximum);
				shader->BindTextureSampler(0, source->second->texture->GetTextureID());
				shader->BindImage(0, preview->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
				shader->DispatchForTexture(previewSize, previewSize);
			}
		}
		shader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	}

	bool TerrainComputePipeline::UpdateParticleErosion(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, float deltaTime)
	{
		if (!m_particleErosionRunning || !m_particleErosionReady) return false;
		m_particleStepAccumulator += std::clamp(deltaTime, 0.0f, 0.1f) * 60.0f * m_terrainPassContext.hydraulicSettings.particleSimulationSpeed;
		int steps = std::min(static_cast<int>(m_particleStepAccumulator), 4);
		if (steps <= 0) return false;
		m_particleStepAccumulator -= static_cast<float>(steps);
		for (int i = 0; i < steps; ++i) StepParticleErosion(masks);
		return true;
	}

	void TerrainComputePipeline::StartParticleErosion()
	{
		if (m_particleErosionReady) m_particleErosionRunning = true;
	}

	void TerrainComputePipeline::PauseParticleErosion()
	{
		m_particleErosionRunning = false;
		m_particleStepAccumulator = 0.0f;
	}

	void TerrainComputePipeline::CancelParticleErosion() { PauseParticleErosion(); }
	bool TerrainComputePipeline::IsParticleErosionActive() const { return m_particleErosionRunning; }
	bool TerrainComputePipeline::IsParticleErosionReady() const { return m_particleErosionReady; }
	float TerrainComputePipeline::GetParticleErosionSpeed() const { return m_terrainPassContext.hydraulicSettings.particleSimulationSpeed; }
	void TerrainComputePipeline::SetParticleErosionSpeed(float speed) { m_terrainPassContext.hydraulicSettings.particleSimulationSpeed = std::clamp(speed, 0.25f, 4.0f); }
	bool TerrainComputePipeline::GetShowErosionParticles() const { return m_terrainPassContext.hydraulicSettings.showErosionParticles; }
	void TerrainComputePipeline::SetShowErosionParticles(bool show) { m_terrainPassContext.hydraulicSettings.showErosionParticles = show; }
	int TerrainComputePipeline::GetActiveParticleCount() const { return m_gpuParticleErosion ? m_gpuParticleErosion->GetParticleCount() : 0; }

	void TerrainComputePipeline::DrawErosionParticles(const glm::mat4& view, const glm::mat4& projection)
	{
		if (!m_terrainPassContext.hydraulicSettings.showErosionParticles || !m_particleErosionReady || !m_gpuParticleErosion) return;
		auto height = m_terrainMasks.find("FinalHeight");
		if (height == m_terrainMasks.end()) return;
		m_gpuParticleErosion->DrawDebug(&view[0][0], &projection[0][0], height->second->texture);
	}

	bool TerrainComputePipeline::StartThermalErosion(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		if (!m_thermalErosionReady || !m_gpuThermalErosion || IsHydrologyRunning()) return false;
		PauseParticleErosion();
		if (!m_thermalErosionInitialized)
		{
			masks.at("ThermalBaseHeight")->texture->CopyFrom(*masks.at("FinalHeight")->texture);
			masks.at("ThermalDebris")->texture->ClearTexture();
			masks.at("ThermalDebrisMask")->texture->ClearTexture();
			masks.at("ThermalErosionMask")->texture->ClearTexture();
			masks.at("ThermalDepositionMask")->texture->ClearTexture();
			m_thermalErosionInitialized = true;
			m_thermalStepCount = 0;
		}
		m_gpuThermalErosion->Reset(m_thermalAmount, m_thermalTalusAngle, m_thermalWeathering);
		m_gpuThermalErosion->Begin(masks.at("FinalHeight")->texture);
		m_thermalErosionRunning = true;
		m_thermalStepAccumulator = 0.0f;
		return true;
	}

	void TerrainComputePipeline::PauseThermalErosion()
	{
		m_thermalErosionRunning = false;
		m_thermalStepAccumulator = 0.0f;
	}

	bool TerrainComputePipeline::StepThermalErosion(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		if (!m_thermalErosionReady || !m_gpuThermalErosion) return false;
		PWG_SIM_EVENT(stepEvent, debug::SimEventType::THERMAL_STEP);
		if (!m_thermalErosionInitialized)
		{
			masks.at("ThermalBaseHeight")->texture->CopyFrom(*masks.at("FinalHeight")->texture);
			masks.at("ThermalDebris")->texture->ClearTexture();
			masks.at("ThermalDebrisMask")->texture->ClearTexture();
			masks.at("ThermalErosionMask")->texture->ClearTexture();
			masks.at("ThermalDepositionMask")->texture->ClearTexture();
			m_gpuThermalErosion->Reset(m_thermalAmount, m_thermalTalusAngle, m_thermalWeathering);
			m_gpuThermalErosion->Begin(masks.at("FinalHeight")->texture);
			m_thermalErosionInitialized = true;
		}

		// Includes the synchronous stability check on every 8th solver step (design F7).
		PWG_SIM_EVENT_GPU_BEGIN(stepEvent, debug::GpuEventMode::ELAPSED);
		const bool stepped = m_gpuThermalErosion->Step(
			masks.at("FinalHeight")->texture,
			m_thermalHardness,
			masks.at("ThermalDebris")->texture,
			masks.at("ThermalDebrisMask")->texture,
			masks.at("ThermalErosionMask")->texture,
			masks.at("ThermalDepositionMask")->texture);
		PWG_SIM_EVENT_GPU_END(stepEvent);
		if (!stepped)
		{
			// No step ran (already stable or nothing to do), so there is no step to report.
			PWG_SIM_EVENT_DISCARD(stepEvent);
			if (m_gpuThermalErosion->IsStable()) PauseThermalErosion();
			return false;
		}

		++m_thermalStepCount;
		PWG_SIM_EVENT_DO(stepEvent,
			stepEvent.Event().stepIndex = m_thermalStepCount;
			stepEvent.Event().solverStep = m_gpuThermalErosion->GetStepCount();
			stepEvent.Event().stable = m_gpuThermalErosion->IsStable();
			stepEvent.Event().refreshStep = (m_thermalStepCount & 3u) == 0u;
			stepEvent.Event().resolution = m_size);
		if ((m_thermalStepCount & 3u) == 0u)
		{
			PWG_SIM_EVENT_GPU(refreshEvent, debug::SimEventType::THERMAL_REFRESH, debug::GpuEventMode::ELAPSED);
			PWG_SIM_EVENT_SET(refreshEvent, stepIndex, m_thermalStepCount);
			RefreshMaskPass(masks);
			RefreshThermalErosionPreviews(masks);
		}
		if (m_gpuThermalErosion->IsStable()) PauseThermalErosion();
		m_terrainMasks = masks;
		InvalidateWaterRenderCache();
		return true;
	}

	bool TerrainComputePipeline::UpdateThermalErosion(
		std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, float deltaTime)
	{
		if (!m_thermalErosionRunning || !m_thermalErosionReady) return false;
		m_thermalStepAccumulator += std::clamp(deltaTime, 0.0f, 0.1f) * 60.0f *
			m_terrainPassContext.thermalSettings.simulationSpeed;
		const int steps = std::min(static_cast<int>(m_thermalStepAccumulator), 4);
		if (steps <= 0) return false;
		m_thermalStepAccumulator -= static_cast<float>(steps);
		bool changed = false;
		for (int i = 0; i < steps && m_thermalErosionRunning; ++i)
		{
			changed = StepThermalErosion(masks) || changed;
		}
		return changed;
	}

	bool TerrainComputePipeline::ResetThermalErosionState(
		std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		PauseThermalErosion();
		if (!m_thermalErosionInitialized) return false;
		masks.at("FinalHeight")->texture->CopyFrom(*masks.at("ThermalBaseHeight")->texture);
		for (const char* name : { "ThermalBaseHeight", "ThermalDebris", "ThermalDebrisMask",
			"ThermalErosionMask", "ThermalDepositionMask" })
		{
			masks.at(name)->texture->ClearTexture();
		}
		m_gpuThermalErosion->Reset(m_thermalAmount, m_thermalTalusAngle, m_thermalWeathering);
		m_thermalErosionInitialized = false;
		m_thermalStepCount = 0;
		RefreshMaskPass(masks);
		RefreshThermalErosionPreviews(masks);
		InvalidateWaterRenderCache();
		return true;
	}

	void TerrainComputePipeline::RefreshThermalErosionPreviews(
		std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		auto shader = m_computeShaders.at("UniversalGraphPreview");
		constexpr int previewSize = 256;
		for (const auto& node : m_universalGraphAsset.nodes)
		{
			if (!node.enabled || node.typeId != "thermalErosion") continue;
			for (const char* port : { "height", "debris", "erosion", "deposition" })
			{
				auto source = masks.find(TempMaskName(node.id, port));
				if (source == masks.end() || !source->second || !source->second->texture) continue;
				auto range = EstimateTerrainGraphOutputRange(m_universalGraphAsset, node.id, port);
				if (!range.valid || std::abs(range.maximum - range.minimum) < 0.0001f)
					range = { 0.0f, 1.0f, true };
				auto preview = EnsureFloatMask(masks, "preview_" + TempMaskName(node.id, port), previewSize);
				shader->ActivateShader();
				shader->SetUniformInt("u_value", 0);
				shader->SetUniformInt("u_size", previewSize);
				shader->SetUniformFloat("u_minValue", range.minimum);
				shader->SetUniformFloat("u_maxValue", range.maximum);
				shader->BindTextureSampler(0, source->second->texture->GetTextureID());
				shader->BindImage(0, preview->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
				shader->DispatchForTexture(previewSize, previewSize);
			}
		}
		shader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	}

	bool TerrainComputePipeline::IsThermalErosionActive() const { return m_thermalErosionRunning; }
	bool TerrainComputePipeline::IsThermalErosionReady() const { return m_thermalErosionReady; }
	bool TerrainComputePipeline::IsThermalErosionStable() const
	{
		return m_thermalErosionInitialized && m_gpuThermalErosion && m_gpuThermalErosion->IsStable();
	}
	bool TerrainComputePipeline::HasThermalErosionState() const { return m_thermalErosionInitialized; }
	float TerrainComputePipeline::GetThermalErosionSpeed() const
	{
		return m_terrainPassContext.thermalSettings.simulationSpeed;
	}
	void TerrainComputePipeline::SetThermalErosionSpeed(float speed)
	{
		m_terrainPassContext.thermalSettings.simulationSpeed = std::clamp(speed, 0.25f, 4.0f);
	}
	std::string TerrainComputePipeline::GetThermalErosionStatus() const
	{
		if (!m_thermalErosionReady) return "Add an enabled Thermal Erosion node";
		if (m_thermalErosionRunning) return "Running";
		if (IsThermalErosionStable()) return "Stable";
		return m_thermalErosionInitialized ? "Paused" : "Ready";
	}

	void TerrainComputePipeline::MarkThermalErosionRestored(
		std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, bool restored)
	{
		PauseThermalErosion();
		m_thermalErosionInitialized = restored;
		m_thermalStepCount = 0;
		for (const auto& node : m_universalGraphAsset.nodes)
		{
			if (!node.enabled || node.typeId != "thermalErosion") continue;
			m_thermalAmount = std::clamp(GetFloatParameter(node, "amount", 0.35f), 0.0f, 1.0f);
			m_thermalTalusAngle = std::clamp(GetFloatParameter(node, "talusAngle", 32.0f), 15.0f, 60.0f);
			m_thermalWeathering = std::clamp(GetFloatParameter(node, "weathering", 0.45f), 0.0f, 1.0f);
			m_thermalErosionReady = m_thermalAmount > 0.0001f;
			masks[TempMaskName(node.id, "height")] = masks.at("FinalHeight");
			masks[TempMaskName(node.id, "debris")] = masks.at("ThermalDebrisMask");
			masks[TempMaskName(node.id, "erosion")] = masks.at("ThermalErosionMask");
			masks[TempMaskName(node.id, "deposition")] = masks.at("ThermalDepositionMask");
			break;
		}
		m_gpuThermalErosion->Reset(m_thermalAmount, m_thermalTalusAngle, m_thermalWeathering);
		if (restored) m_gpuThermalErosion->Begin(masks.at("FinalHeight")->texture);
	}

	void TerrainComputePipeline::InvalidateWaterRenderCache()
	{
		if (m_waterClipmapRenderer) m_waterClipmapRenderer->Invalidate();
	}

	void TerrainComputePipeline::DrawWaterClipmap(const std::shared_ptr<Shader>& shader, const glm::vec3& cameraPosition,
		const std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		if (!m_waterClipmapRenderer || !shader) return;
		auto findTexture = [&](const char* name) -> std::shared_ptr<Texture>
		{
			auto it = masks.find(name);
			return it != masks.end() && it->second ? it->second->texture : nullptr;
		};
		m_waterClipmapRenderer->Refresh(
			findTexture("FinalHeight"), findTexture("WaterMask"),
			findTexture("FlowAccumulationMask"), findTexture("RiverChannelMask"),
			m_terrainPassContext.hydraulicSettings.minVisibleWaterDepth);
		m_waterClipmapRenderer->Draw(shader, cameraPosition);
	}

	bool TerrainComputePipeline::GenerateHydrology(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		if (!m_drainageNetworkGenerator || m_drainageNetworkGenerator->IsRunning() ||
			IsParticleErosionActive() || IsThermalErosionActive()) return false;
		if (m_hydrologyGenerated) ResetHydrology(masks);

		DrainageNetworkSettings settings;
		bool foundDrainageNode = false;
		for (const auto& node : m_universalGraphAsset.nodes)
		{
			if (!node.enabled || node.typeId != "drainageNetwork") continue;
			settings.riverAmount = GetFloatParameter(node, "riverAmount", 0.5f);
			settings.lakeAmount = GetFloatParameter(node, "lakeAmount", 0.5f);
			settings.seed = GetIntParameter(node, "seed", 1337);
			foundDrainageNode = true;
			break;
		}
		if (!foundDrainageNode)
		{
			m_hydrologyStatus = "Add a Drainage Network node first";
			return false;
		}
		PWG_SIM_CALL(BeginHydrologyRun());
		PWG_SIM_EVENT(readbackEvent, debug::SimEventType::HYDROLOGY_READBACK);
		PWG_SIM_EVENT_SET(readbackEvent, resolution, m_size);
		auto height = masks.at("FinalHeight")->texture->ReadFloatData();
		PWG_SIM_EVENT_END(readbackEvent);
		if (!m_drainageNetworkGenerator->Start(std::move(height), m_size, settings)) return false;
		PWG_SIM_CALL(MarkHydrologyStarted());
		m_hydrologyStatus = "Preparing heightmap";
		return true;
	}

	bool TerrainComputePipeline::UpdateHydrology(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		if (!m_drainageNetworkGenerator) return false;
		if (m_drainageNetworkGenerator->IsRunning()) m_hydrologyStatus = m_drainageNetworkGenerator->GetStage();
		DrainageNetworkResult result;
		if (!m_drainageNetworkGenerator->TryTakeResult(result)) return false;
		PublishHydrologyTiming(result);
		if (!result.IsValid())
		{
			m_hydrologyStatus = result.cancelled ? "Hydrology cancelled" : "Hydrology generation failed";
			return false;
		}

		PWG_SIM_EVENT(uploadEvent, debug::SimEventType::HYDROLOGY_UPLOAD);
		PWG_SIM_EVENT_SET(uploadEvent, resolution, m_size);
		auto upload = [&](const char* name, const std::vector<float>& values)
		{
			masks.at(name)->texture->UpdateFloatData(values, m_size, m_size);
		};
		upload("HydrologyBaseHeight", result.baseHeight);
		upload("FinalHeight", result.finalHeight);
		upload("WaterMask", result.water);
		upload("FlowAccumulationMask", result.flow);
		upload("RiverChannelMask", result.rivers);
		upload("LakeMask", result.lakes);
		upload("WaterVelocityMap", result.velocity);
		upload("RiverCarveDelta", result.carveDelta);

		for (const auto& node : m_universalGraphAsset.nodes)
		{
			if (!node.enabled || node.typeId != "drainageNetwork") continue;
			masks[TempMaskName(node.id, "water")] = masks.at("WaterMask");
			masks[TempMaskName(node.id, "flow")] = masks.at("FlowAccumulationMask");
			masks[TempMaskName(node.id, "rivers")] = masks.at("RiverChannelMask");
			masks[TempMaskName(node.id, "lakes")] = masks.at("LakeMask");
		}
		RefreshHydrologyGraphOutputs(masks);
		m_graph->ExecutePasses(TerrainPassTypes::MASK, masks, m_terrainPassContext);
		PWG_SIM_EVENT_END(uploadEvent);
		m_terrainMasks = masks;
		m_hydrologyGenerated = true;
		m_hydrologyStatus = "Hydrology ready";
		InvalidateWaterRenderCache();
		return true;
	}

	void TerrainComputePipeline::CancelHydrology()
	{
		if (m_drainageNetworkGenerator) m_drainageNetworkGenerator->Cancel();
	}

	bool TerrainComputePipeline::ResetHydrology(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		CancelHydrology();
		if (!m_hydrologyGenerated) return false;
		masks.at("FinalHeight")->texture->CopyFrom(*masks.at("HydrologyBaseHeight")->texture);
		for (const char* name : { "WaterMask", "FlowAccumulationMask", "RiverChannelMask", "LakeMask", "WaterVelocityMap", "RiverCarveDelta" })
			masks.at(name)->texture->ClearTexture();
		RefreshHydrologyGraphOutputs(masks);
		m_graph->ExecutePasses(TerrainPassTypes::MASK, masks, m_terrainPassContext);
		m_terrainMasks = masks;
		m_hydrologyGenerated = false;
		m_hydrologyStatus = "Hydrology ready to generate";
		InvalidateWaterRenderCache();
		return true;
	}

	void TerrainComputePipeline::RefreshHydrologyGraphOutputs(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
	{
		auto validation = ValidateTerrainGraphAsset(m_universalGraphAsset);
		if (!validation.valid) return;

		std::unordered_map<int, bool> dependsOnHydrology;
		auto inputFor = [&](const TerrainGraphNodeInstance& node, const std::string& portId) -> std::shared_ptr<TerrainMask>
		{
			const auto* link = m_universalGraphAsset.FindInputLink(node.id, portId);
			if (!link) return nullptr;
			auto found = masks.find(TempMaskName(link->fromNodeId, link->fromPortId));
			return found != masks.end() ? found->second : nullptr;
		};
		auto makeOutput = [&](const TerrainGraphNodeInstance& node, const std::string& portId)
		{
			return EnsureFloatMask(masks, TempMaskName(node.id, portId), m_size);
		};

		for (int nodeId : validation.executionOrder)
		{
			const auto* node = m_universalGraphAsset.FindNode(nodeId);
			if (!node || !node->enabled) continue;
			if (node->typeId == "drainageNetwork")
			{
				dependsOnHydrology[nodeId] = true;
				continue;
			}

			bool dependent = false;
			for (const auto& link : m_universalGraphAsset.links)
			{
				if (link.toNodeId == nodeId && dependsOnHydrology[link.fromNodeId])
				{
					dependent = true;
					break;
				}
			}
			dependsOnHydrology[nodeId] = dependent;
			if (!dependent) continue;

			if (node->typeId == "add" || node->typeId == "subtract" || node->typeId == "multiply" ||
				node->typeId == "divide" || node->typeId == "min" || node->typeId == "max")
			{
				auto a = inputFor(*node, "a");
				auto b = inputFor(*node, "b");
				if (!a || !b) continue;
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
			}
			else if (node->typeId == "lerp")
			{
				auto a = inputFor(*node, "a");
				auto b = inputFor(*node, "b");
				auto t = inputFor(*node, "t");
				if (!a || !b) continue;
				auto output = makeOutput(*node, "result");
				auto shader = m_computeShaders["UniversalLerp"];
				shader->ActivateShader();
				a->texture->Bind(0);
				b->texture->Bind(1);
				if (t) t->texture->Bind(2);
				shader->SetUniformInt("u_a", 0);
				shader->SetUniformInt("u_b", 1);
				shader->SetUniformInt("u_t", 2);
				shader->SetUniformInt("u_size", m_size);
				shader->SetUniformFloat("u_defaultT", 0.5f);
				shader->SetUniformInt("u_useTextureT", t ? 1 : 0);
				shader->BindImage(0, output->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
				shader->DispatchForTexture(m_size, m_size);
				shader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
			}
			else if (node->typeId == "clamp" || node->typeId == "normalize" || node->typeId == "power" ||
				node->typeId == "thresholdMask" || node->typeId == "heightBandMask" || node->typeId == "abs" ||
				node->typeId == "invert" || node->typeId == "fitRange" || node->typeId == "smoothstep" ||
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
				if (!value) continue;
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
				shader->SetUniformFloat("u_min", node->typeId == "fitRange" ? GetFloatParameter(*node, "inMin", 0.0f) : GetFloatParameter(*node, "min", 0.0f));
				shader->SetUniformFloat("u_max", node->typeId == "fitRange" ? GetFloatParameter(*node, "inMax", 1.0f) : GetFloatParameter(*node, "max", 1.0f));
				shader->SetUniformFloat("u_power", GetFloatParameter(*node, "power", GetFloatParameter(*node, "pivot", 1.0f)));
				shader->SetUniformFloat("u_outMin", GetFloatParameter(*node, "outMin", 0.0f));
				shader->SetUniformFloat("u_outMax", GetFloatParameter(*node, "outMax", 1.0f));
				shader->SetUniformInt("u_steps", GetIntParameter(*node, "steps", 8));
				shader->BindImage(0, output->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
				shader->DispatchForTexture(m_size, m_size);
				shader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
			}
			else if (node->typeId == "slopeMask" || node->typeId == "curvatureMask" || node->typeId == "blurMask" ||
				node->typeId == "erodeMask" || node->typeId == "dilateMask")
			{
				const bool usesMaskInput = node->typeId == "blurMask" || node->typeId == "erodeMask" || node->typeId == "dilateMask";
				const char* inputPort = usesMaskInput ? "mask" : "height";
				auto value = inputFor(*node, inputPort);
				if (!value) continue;
				auto output = makeOutput(*node, "mask");
				int operation = 0;
				if (node->typeId == "curvatureMask") operation = 1;
				else if (node->typeId == "blurMask") operation = 2;
				else if (node->typeId == "erodeMask") operation = 3;
				else if (node->typeId == "dilateMask") operation = 4;
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
			}
			else if (node->typeId == "outputMask")
			{
				auto source = inputFor(*node, "mask");
				if (!source) continue;
				auto target = EnsureFloatMask(masks, GetStringParameter(*node, "target", "FinalMask"), m_size);
				target->texture->CopyFrom(*source->texture);
			}
		}

		auto previewShader = m_computeShaders["UniversalGraphPreview"];
		for (const auto& node : m_universalGraphAsset.nodes)
		{
			if (!node.enabled || !dependsOnHydrology[node.id]) continue;
			const auto* type = FindTerrainGraphNodeType(node.typeId);
			if (!type) continue;
			for (const auto& output : type->outputs)
			{
				if (output.valueType != TerrainGraphValueType::FloatMask) continue;
				auto source = masks.find(TempMaskName(node.id, output.id));
				if (source == masks.end() || !source->second || !source->second->texture) continue;
				auto preview = EnsureFloatMask(masks, "preview_" + TempMaskName(node.id, output.id), 256);
				previewShader->ActivateShader();
				source->second->texture->Bind(0);
				previewShader->SetUniformInt("u_value", 0);
				previewShader->SetUniformInt("u_size", 256);
				previewShader->SetUniformFloat("u_minValue", 0.0f);
				previewShader->SetUniformFloat("u_maxValue", 1.0f);
				previewShader->BindImage(0, preview->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
				previewShader->DispatchForTexture(256, 256);
				previewShader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
			}
		}
	}

	bool TerrainComputePipeline::IsHydrologyRunning() const
	{
		return m_drainageNetworkGenerator && m_drainageNetworkGenerator->IsRunning();
	}

	float TerrainComputePipeline::GetHydrologyProgress() const
	{
		return m_drainageNetworkGenerator ? m_drainageNetworkGenerator->GetProgress() : 0.0f;
	}

	std::string TerrainComputePipeline::GetHydrologyStatus() const { return m_hydrologyStatus; }
	void TerrainComputePipeline::MarkHydrologyRestored(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, bool restored)
	{
		m_hydrologyGenerated = restored;
		m_hydrologyStatus = restored ? "Hydrology restored from project" : "Hydrology ready to generate";
		if (!restored) return;
		for (const auto& node : m_universalGraphAsset.nodes)
		{
			if (!node.enabled || node.typeId != "drainageNetwork") continue;
			masks[TempMaskName(node.id, "water")] = masks.at("WaterMask");
			masks[TempMaskName(node.id, "flow")] = masks.at("FlowAccumulationMask");
			masks[TempMaskName(node.id, "rivers")] = masks.at("RiverChannelMask");
			masks[TempMaskName(node.id, "lakes")] = masks.at("LakeMask");
		}
		RefreshHydrologyGraphOutputs(masks);
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

			if (node.typeId == "drainageNetwork")
			{
				settings.riverAmount = GetFloatParameter(node, "riverAmount", settings.riverAmount);
				settings.lakeAmount = GetFloatParameter(node, "lakeAmount", settings.lakeAmount);
				settings.hydrologySeed = GetIntParameter(node, "seed", settings.hydrologySeed);
			}
		}
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
		terrainMasks["ErosionMask"] = std::make_shared<TerrainMask>("ErosionMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["SedimentMask"] = std::make_shared<TerrainMask>("SedimentMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["DepositMask"] = std::make_shared<TerrainMask>("DepositMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["ErosionDelta"] = std::make_shared<TerrainMask>("ErosionDelta", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["FlowAccumulationMask"] = std::make_shared<TerrainMask>("FlowAccumulationMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["RiverChannelMask"] = std::make_shared<TerrainMask>("RiverChannelMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["LakeMask"] = std::make_shared<TerrainMask>("LakeMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["WaterVelocityMap"] = std::make_shared<TerrainMask>("WaterVelocityMap", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT2), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["RiverCarveDelta"] = std::make_shared<TerrainMask>("RiverCarveDelta", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["HydrologyBaseHeight"] = std::make_shared<TerrainMask>("HydrologyBaseHeight", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::HEIGHT, 1.0f);
		terrainMasks["ErosionWaterMask"] = std::make_shared<TerrainMask>("ErosionWaterMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["ErosionFlowMask"] = std::make_shared<TerrainMask>("ErosionFlowMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["ErosionChannelMask"] = std::make_shared<TerrainMask>("ErosionChannelMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["ThermalBaseHeight"] = std::make_shared<TerrainMask>("ThermalBaseHeight", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::HEIGHT, 1.0f);
		terrainMasks["ThermalDebris"] = std::make_shared<TerrainMask>("ThermalDebris", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::HEIGHT, 1.0f);
		terrainMasks["ThermalDebrisMask"] = std::make_shared<TerrainMask>("ThermalDebrisMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["ThermalErosionMask"] = std::make_shared<TerrainMask>("ThermalErosionMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);
		terrainMasks["ThermalDepositionMask"] = std::make_shared<TerrainMask>("ThermalDepositionMask", std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT), MaskUsage::WEIGHTS, 1.0f);

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
	ThermalSimulationSettings& TerrainComputePipeline::GetThermalSettings()
	{
		return m_terrainPassContext.thermalSettings;
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


