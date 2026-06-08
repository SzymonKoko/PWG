#include "projectSerializer.h"

#include "scene/editorScene.h"
#include "scene/objects/sunObject.h"
#include "terrain/materials/terrainMaterialLibrary.h"
#include "terrain/graph/passes/terrainPass.h"
#include "terrain/graph/terrainPassContext.h"
#include "terrain/chunk/chunkManager.h"
#include "yaml-cpp/yaml.h"

#include <fstream>
#include <unordered_map>
#include <filesystem>
#include <cstdint>

namespace pwg
{
	namespace
	{
		YAML::Node Vec3Node(const glm::vec3& value)
		{
			YAML::Node node;
			node["x"] = value.x;
			node["y"] = value.y;
			node["z"] = value.z;
			return node;
		}

		std::string RoleName(TerrainMaterialRole role)
		{
			switch (role)
			{
			case TerrainMaterialRole::Grass:
				return "Grass";
			case TerrainMaterialRole::Rock:
				return "Rock";
			case TerrainMaterialRole::Sediment:
				return "Sediment";
			case TerrainMaterialRole::Snow:
				return "Snow";
			default:
				return "Unknown";
			}
		}

		TerrainMaterialRole ReadRole(const std::string& value)
		{
			if (value == "Rock")
			{
				return TerrainMaterialRole::Rock;
			}
			if (value == "Sediment")
			{
				return TerrainMaterialRole::Sediment;
			}
			if (value == "Snow")
			{
				return TerrainMaterialRole::Snow;
			}
			return TerrainMaterialRole::Grass;
		}

		template<typename T>
		void ReadValue(const YAML::Node& node, const char* key, T& value)
		{
			if (node && node[key])
			{
				value = node[key].as<T>();
			}
		}

		YAML::Node WriteTerrainMaterials(const TerrainMaterialLibrary& library)
		{
			YAML::Node node;
			for (const auto& material : library.GetMaterials())
			{
				YAML::Node materialNode;
				materialNode["name"] = material.name;
				materialNode["role"] = RoleName(material.role);
				materialNode["folder"] = material.folder;
				materialNode["albedo"] = material.albedo;
				materialNode["normal"] = material.normal;
				materialNode["roughness"] = material.roughness;
				materialNode["ao"] = material.ao;
				materialNode["height"] = material.height;
				materialNode["tiling"] = material.tiling;
				materialNode["normalStrength"] = material.normalStrength;
				materialNode["roughnessScale"] = material.roughnessScale;
				materialNode["heightStrength"] = material.heightStrength;
				node.push_back(materialNode);
			}
			return node;
		}

		YAML::Node WriteSkybox(const SkyboxDefinition& skybox)
		{
			YAML::Node node;
			node["name"] = skybox.name;
			node["folder"] = skybox.folder;
			for (const auto& face : skybox.faces)
			{
				node["faces"].push_back(face);
			}
			for (const auto& rotation : skybox.rotations)
			{
				node["rotations"].push_back(rotation);
			}
			for (const auto& flip : skybox.flipHorizontal)
			{
				node["flipHorizontal"].push_back(flip);
			}
			for (const auto& flip : skybox.flipVertical)
			{
				node["flipVertical"].push_back(flip);
			}
			return node;
		}

		void ReadSkybox(const YAML::Node& node, EditorScene& scene)
		{
			if (!node)
			{
				return;
			}

			SkyboxDefinition skybox = scene.GetSkyboxDefinition();
			ReadValue(node, "name", skybox.name);
			ReadValue(node, "folder", skybox.folder);

			auto faces = node["faces"];
			if (faces && faces.IsSequence() && faces.size() == skybox.faces.size())
			{
				for (std::size_t i = 0; i < skybox.faces.size(); ++i)
				{
					skybox.faces[i] = faces[i].as<std::string>();
				}
			}
			auto rotations = node["rotations"];
			if (rotations && rotations.IsSequence() && rotations.size() == skybox.rotations.size())
			{
				for (std::size_t i = 0; i < skybox.rotations.size(); ++i)
				{
					skybox.rotations[i] = rotations[i].as<int>();
				}
			}
			auto flipHorizontal = node["flipHorizontal"];
			if (flipHorizontal && flipHorizontal.IsSequence() && flipHorizontal.size() == skybox.flipHorizontal.size())
			{
				for (std::size_t i = 0; i < skybox.flipHorizontal.size(); ++i)
				{
					skybox.flipHorizontal[i] = flipHorizontal[i].as<bool>();
				}
			}
			auto flipVertical = node["flipVertical"];
			if (flipVertical && flipVertical.IsSequence() && flipVertical.size() == skybox.flipVertical.size())
			{
				for (std::size_t i = 0; i < skybox.flipVertical.size(); ++i)
				{
					skybox.flipVertical[i] = flipVertical[i].as<bool>();
				}
			}

			scene.SetSkyboxDefinition(skybox);
		}

		YAML::Node WriteEditorUi(EditorScene& scene)
		{
			YAML::Node node;
			node["showMainView"] = true;
			node["showTerrainControls"] = scene.ShowTerrainControls();
			node["showWorld"] = scene.ShowWorld();
			node["showOutliner"] = scene.ShowOutliner();
			node["showDebugWindow"] = scene.ShowDebugWindow();
			node["showMaterialLibrary"] = scene.ShowMaterialLibrary();
			node["imguiLayout"] = scene.GetProjectLayoutIni();
			return node;
		}

		void ReadEditorUi(const YAML::Node& node, EditorScene& scene)
		{
			if (!node)
			{
				return;
			}

			scene.ShowMainView() = true;
			ReadValue(node, "showTerrainControls", scene.ShowTerrainControls());
			ReadValue(node, "showWorld", scene.ShowWorld());
			ReadValue(node, "showOutliner", scene.ShowOutliner());
			ReadValue(node, "showDebugWindow", scene.ShowDebugWindow());
			ReadValue(node, "showMaterialLibrary", scene.ShowMaterialLibrary());
			if (node["showLightControls"] || node["showEnvironmentWindow"])
			{
				bool legacyLight = true;
				bool legacyEnvironment = true;
				ReadValue(node, "showLightControls", legacyLight);
				ReadValue(node, "showEnvironmentWindow", legacyEnvironment);
				scene.ShowWorld() = legacyLight || legacyEnvironment;
			}

			std::string layout;
			ReadValue(node, "imguiLayout", layout);
			scene.SetProjectLayoutIni(layout);
		}

		void ReadTerrainMaterials(const YAML::Node& node, TerrainMaterialLibrary& library)
		{
			if (!node || !node.IsSequence())
			{
				return;
			}

			auto materials = library.GetMaterials();
			for (const auto& materialNode : node)
			{
				TerrainMaterialRole role = ReadRole(materialNode["role"] ? materialNode["role"].as<std::string>() : "Grass");
				int index = static_cast<int>(role);
				auto& material = materials[index];
				ReadValue(materialNode, "name", material.name);
				material.role = role;
				ReadValue(materialNode, "folder", material.folder);
				ReadValue(materialNode, "albedo", material.albedo);
				ReadValue(materialNode, "normal", material.normal);
				ReadValue(materialNode, "roughness", material.roughness);
				ReadValue(materialNode, "ao", material.ao);
				ReadValue(materialNode, "height", material.height);
				ReadValue(materialNode, "tiling", material.tiling);
				ReadValue(materialNode, "normalStrength", material.normalStrength);
				ReadValue(materialNode, "roughnessScale", material.roughnessScale);
				ReadValue(materialNode, "heightStrength", material.heightStrength);
			}
			library.SetMaterials(materials);
			library.ReloadTerrainMaterials();
		}

		glm::vec3 ReadVec3(const YAML::Node& node, const glm::vec3& fallback)
		{
			if (!node)
			{
				return fallback;
			}
			return {
				node["x"] ? node["x"].as<float>() : fallback.x,
				node["y"] ? node["y"].as<float>() : fallback.y,
				node["z"] ? node["z"].as<float>() : fallback.z
			};
		}

		void WriteParameter(YAML::Node& parent, const UIParameters& param)
		{
			switch (param.type)
			{
			case ParameterType::Float:
				parent[param.name] = *static_cast<float*>(param.data);
				break;
			case ParameterType::Int:
			case ParameterType::List:
				parent[param.name] = *static_cast<int*>(param.data);
				break;
			case ParameterType::Bool:
				parent[param.name] = *static_cast<bool*>(param.data);
				break;
			default:
				break;
			}
		}

		void ReadParameter(const YAML::Node& parent, const UIParameters& param)
		{
			if (!parent || !parent[param.name])
			{
				return;
			}

			switch (param.type)
			{
			case ParameterType::Float:
				*static_cast<float*>(param.data) = parent[param.name].as<float>();
				break;
			case ParameterType::Int:
			case ParameterType::List:
				*static_cast<int*>(param.data) = parent[param.name].as<int>();
				break;
			case ParameterType::Bool:
				*static_cast<bool*>(param.data) = parent[param.name].as<bool>();
				break;
			default:
				break;
			}
		}

		YAML::Node WriteHydraulicSettings(const HydraulicSimulationSettings& settings)
		{
			YAML::Node node;
			node["stepsPerUpdate"] = settings.stepsPerUpdate;
			node["waterStepsPerSecond"] = settings.waterStepsPerSecond;
			node["waterSubsteps"] = settings.waterSubsteps;
			node["bakeParticleSteps"] = settings.bakeParticleSteps;
			node["bakeStepsPerFrame"] = settings.bakeStepsPerFrame;
			node["bakeUploadInterval"] = settings.bakeUploadInterval;
			node["erosionEnabled"] = settings.erosionEnabled;
			node["autoWaterSimulation"] = settings.autoWaterSimulation;
			node["particleErosionEnabled"] = settings.particleErosionEnabled;
			node["showWaterSurface"] = settings.showWaterSurface;
			node["waterRefractionEnabled"] = settings.waterRefractionEnabled;
			node["useDiagonalFlow"] = settings.useDiagonalFlow;
			node["rainRate"] = settings.rainRate;
			node["rainElevationBias"] = settings.rainElevationBias;
			node["rainMinHeight"] = settings.rainMinHeight;
			node["rainMaxHeight"] = settings.rainMaxHeight;
			node["pipeFlow"] = settings.pipeFlow;
			node["pipeDamping"] = settings.pipeDamping;
			node["flowRate"] = settings.pipeFlow;
			node["flowInertia"] = settings.pipeDamping;
			node["flowConcentration"] = settings.flowConcentration;
			node["erosionStrength"] = settings.erosionStrength;
			node["flatteningStrength"] = settings.flatteningStrength;
			node["depositionStrength"] = settings.depositionStrength;
			node["maxDepositHeight"] = settings.maxDepositHeight;
			node["boundaryDrain"] = settings.boundaryDrain;
			node["lakeLeveling"] = settings.lakeLeveling;
			node["maxWaterDepth"] = settings.maxWaterDepth;
			node["maxSurfaceDrop"] = settings.maxSurfaceDrop;
			node["evaporation"] = settings.evaporation;
			node["infiltrationRate"] = settings.infiltrationRate;
			node["lakeInfiltrationScale"] = settings.lakeInfiltrationScale;
			node["sedimentCapacity"] = settings.sedimentCapacity;
			node["erosionRate"] = settings.erosionStrength;
			node["slopeErosionRate"] = settings.slopeErosionRate;
			node["slopeErosionMinSlope"] = settings.slopeErosionMinSlope;
			node["slopeErosionWaterScale"] = settings.slopeErosionWaterScale;
			node["depositionRate"] = settings.depositionStrength;
			node["depositDiffusion"] = settings.depositDiffusion;
			node["valleyDepositionRate"] = settings.valleyDepositionRate;
			node["valleyDepositionSlope"] = settings.valleyDepositionSlope;
			node["steepSlopeDepositionReduction"] = settings.steepSlopeDepositionReduction;
			node["depositionSlopeStart"] = settings.depositionSlopeStart;
			node["depositionSlopeEnd"] = settings.depositionSlopeEnd;
			node["minWaterToErode"] = settings.minWaterToErode;
			node["minFlowToErode"] = settings.minFlowToErode;
			node["maxHeightChange"] = settings.maxHeightChange;
			node["maxErosionDepth"] = settings.maxErosionDepth;
			node["maxDepositionHeight"] = settings.maxDepositionHeight;
			node["lakeDeposition"] = settings.lakeDeposition;
			node["lakeMinDepth"] = settings.lakeMinDepth;
			node["lakeBedSmoothing"] = settings.lakeBedSmoothing;
			node["lakeWaterSmoothing"] = settings.lakeWaterSmoothing;
			node["channelSmoothing"] = settings.channelSmoothing;
			node["flowAccumulationGrowth"] = settings.flowAccumulationGrowth;
			node["flowAccumulationDecay"] = settings.flowAccumulationDecay;
			node["riverChannelGrowth"] = settings.riverChannelGrowth;
			node["riverChannelDecay"] = settings.riverChannelDecay;
			node["riverChannelInfluence"] = settings.riverChannelInfluence;
			node["riverChannelDepth"] = settings.riverChannelDepth;
			node["particlesPerStep"] = settings.particlesPerStep;
			node["dropletLifetime"] = settings.dropletLifetime;
			node["particleSeed"] = settings.particleSeed;
			node["particleInertia"] = settings.particleInertia;
			node["particleGravity"] = settings.particleGravity;
			node["particleInitialWater"] = settings.particleInitialWater;
			node["particleEvaporation"] = settings.particleEvaporation;
			node["particleMinSlope"] = settings.particleMinSlope;
			node["particleCapacity"] = settings.particleCapacity;
			node["particleErosionStrength"] = settings.particleErosionStrength;
			node["particleDepositionStrength"] = settings.particleDepositionStrength;
			node["particleBrushRadius"] = settings.particleBrushRadius;
			node["particleDepositRadius"] = settings.particleDepositRadius;
			node["particleDepositSlopeStart"] = settings.particleDepositSlopeStart;
			node["particleDepositSlopeEnd"] = settings.particleDepositSlopeEnd;
			node["particleDepositValleyBoost"] = settings.particleDepositValleyBoost;
			node["particleDepositShelfBoost"] = settings.particleDepositShelfBoost;
			node["particleDepositMinSuitability"] = settings.particleDepositMinSuitability;
			node["looseSedimentErosionStrength"] = settings.looseSedimentErosionStrength;
			node["looseSedimentSmoothing"] = settings.looseSedimentSmoothing;
			node["particleChannelStrength"] = settings.particleChannelStrength;
			node["particleFlowStrength"] = settings.particleFlowStrength;
			node["lakeFillEnabled"] = settings.lakeFillEnabled;
			node["lakeMinFlow"] = settings.lakeMinFlow;
			node["lakeMaxDepth"] = settings.lakeMaxDepth;
			return node;
		}

		void ReadHydraulicSettings(const YAML::Node& node, HydraulicSimulationSettings& settings)
		{
			ReadValue(node, "stepsPerUpdate", settings.stepsPerUpdate);
			ReadValue(node, "waterStepsPerSecond", settings.waterStepsPerSecond);
			ReadValue(node, "waterSubsteps", settings.waterSubsteps);
			ReadValue(node, "bakeParticleSteps", settings.bakeParticleSteps);
			ReadValue(node, "bakeStepsPerFrame", settings.bakeStepsPerFrame);
			ReadValue(node, "bakeUploadInterval", settings.bakeUploadInterval);
			ReadValue(node, "erosionEnabled", settings.erosionEnabled);
			ReadValue(node, "autoWaterSimulation", settings.autoWaterSimulation);
			ReadValue(node, "particleErosionEnabled", settings.particleErosionEnabled);
			ReadValue(node, "showWaterSurface", settings.showWaterSurface);
			ReadValue(node, "waterRefractionEnabled", settings.waterRefractionEnabled);
			ReadValue(node, "useDiagonalFlow", settings.useDiagonalFlow);
			ReadValue(node, "rainRate", settings.rainRate);
			ReadValue(node, "rainElevationBias", settings.rainElevationBias);
			ReadValue(node, "rainMinHeight", settings.rainMinHeight);
			ReadValue(node, "rainMaxHeight", settings.rainMaxHeight);
			ReadValue(node, "pipeFlow", settings.pipeFlow);
			ReadValue(node, "pipeDamping", settings.pipeDamping);
			ReadValue(node, "flowRate", settings.flowRate);
			ReadValue(node, "flowInertia", settings.flowInertia);
			if (!node["pipeFlow"] && node["flowRate"])
			{
				settings.pipeFlow = settings.flowRate;
			}
			if (!node["pipeDamping"] && node["flowInertia"])
			{
				settings.pipeDamping = settings.flowInertia;
			}
			ReadValue(node, "flowConcentration", settings.flowConcentration);
			ReadValue(node, "erosionStrength", settings.erosionStrength);
			ReadValue(node, "flatteningStrength", settings.flatteningStrength);
			ReadValue(node, "depositionStrength", settings.depositionStrength);
			ReadValue(node, "maxDepositHeight", settings.maxDepositHeight);
			ReadValue(node, "boundaryDrain", settings.boundaryDrain);
			ReadValue(node, "lakeLeveling", settings.lakeLeveling);
			ReadValue(node, "maxWaterDepth", settings.maxWaterDepth);
			ReadValue(node, "maxSurfaceDrop", settings.maxSurfaceDrop);
			ReadValue(node, "evaporation", settings.evaporation);
			ReadValue(node, "infiltrationRate", settings.infiltrationRate);
			ReadValue(node, "lakeInfiltrationScale", settings.lakeInfiltrationScale);
			ReadValue(node, "sedimentCapacity", settings.sedimentCapacity);
			ReadValue(node, "erosionRate", settings.erosionRate);
			if (!node["erosionStrength"] && node["erosionRate"])
			{
				settings.erosionStrength = settings.erosionRate;
			}
			ReadValue(node, "slopeErosionRate", settings.slopeErosionRate);
			ReadValue(node, "slopeErosionMinSlope", settings.slopeErosionMinSlope);
			ReadValue(node, "slopeErosionWaterScale", settings.slopeErosionWaterScale);
			ReadValue(node, "depositionRate", settings.depositionRate);
			if (!node["depositionStrength"] && node["depositionRate"])
			{
				settings.depositionStrength = settings.depositionRate;
			}
			ReadValue(node, "depositDiffusion", settings.depositDiffusion);
			ReadValue(node, "valleyDepositionRate", settings.valleyDepositionRate);
			ReadValue(node, "valleyDepositionSlope", settings.valleyDepositionSlope);
			ReadValue(node, "steepSlopeDepositionReduction", settings.steepSlopeDepositionReduction);
			ReadValue(node, "depositionSlopeStart", settings.depositionSlopeStart);
			ReadValue(node, "depositionSlopeEnd", settings.depositionSlopeEnd);
			ReadValue(node, "minWaterToErode", settings.minWaterToErode);
			ReadValue(node, "minFlowToErode", settings.minFlowToErode);
			ReadValue(node, "maxHeightChange", settings.maxHeightChange);
			ReadValue(node, "maxErosionDepth", settings.maxErosionDepth);
			ReadValue(node, "maxDepositionHeight", settings.maxDepositionHeight);
			ReadValue(node, "lakeDeposition", settings.lakeDeposition);
			ReadValue(node, "lakeMinDepth", settings.lakeMinDepth);
			ReadValue(node, "lakeBedSmoothing", settings.lakeBedSmoothing);
			ReadValue(node, "lakeWaterSmoothing", settings.lakeWaterSmoothing);
			ReadValue(node, "channelSmoothing", settings.channelSmoothing);
			ReadValue(node, "flowAccumulationGrowth", settings.flowAccumulationGrowth);
			ReadValue(node, "flowAccumulationDecay", settings.flowAccumulationDecay);
			ReadValue(node, "riverChannelGrowth", settings.riverChannelGrowth);
			ReadValue(node, "riverChannelDecay", settings.riverChannelDecay);
			ReadValue(node, "riverChannelInfluence", settings.riverChannelInfluence);
			ReadValue(node, "riverChannelDepth", settings.riverChannelDepth);
			ReadValue(node, "particlesPerStep", settings.particlesPerStep);
			ReadValue(node, "dropletLifetime", settings.dropletLifetime);
			ReadValue(node, "particleSeed", settings.particleSeed);
			ReadValue(node, "particleInertia", settings.particleInertia);
			ReadValue(node, "particleGravity", settings.particleGravity);
			ReadValue(node, "particleInitialWater", settings.particleInitialWater);
			ReadValue(node, "particleEvaporation", settings.particleEvaporation);
			ReadValue(node, "particleMinSlope", settings.particleMinSlope);
			ReadValue(node, "particleCapacity", settings.particleCapacity);
			ReadValue(node, "particleErosionStrength", settings.particleErosionStrength);
			ReadValue(node, "particleDepositionStrength", settings.particleDepositionStrength);
			ReadValue(node, "particleBrushRadius", settings.particleBrushRadius);
			ReadValue(node, "particleDepositRadius", settings.particleDepositRadius);
			ReadValue(node, "particleDepositSlopeStart", settings.particleDepositSlopeStart);
			ReadValue(node, "particleDepositSlopeEnd", settings.particleDepositSlopeEnd);
			ReadValue(node, "particleDepositValleyBoost", settings.particleDepositValleyBoost);
			ReadValue(node, "particleDepositShelfBoost", settings.particleDepositShelfBoost);
			ReadValue(node, "particleDepositMinSuitability", settings.particleDepositMinSuitability);
			ReadValue(node, "looseSedimentErosionStrength", settings.looseSedimentErosionStrength);
			ReadValue(node, "looseSedimentSmoothing", settings.looseSedimentSmoothing);
			ReadValue(node, "particleChannelStrength", settings.particleChannelStrength);
			ReadValue(node, "particleFlowStrength", settings.particleFlowStrength);
			ReadValue(node, "lakeFillEnabled", settings.lakeFillEnabled);
			ReadValue(node, "lakeMinFlow", settings.lakeMinFlow);
			ReadValue(node, "lakeMaxDepth", settings.lakeMaxDepth);
		}
	}

	namespace
	{
		std::string SimulationSidecarPath(const std::string& projectPath)
		{
			std::filesystem::path p(projectPath);
			p.replace_extension(".pwgsim");
			return p.string();
		}

		bool WriteSimulationSidecar(const std::string& path, const SimulationStateSnapshot& snapshot)
		{
			if (!snapshot.HasData())
			{
				return false;
			}

			std::ofstream out(path, std::ios::binary);
			if (!out.is_open())
			{
				return false;
			}

			const char magic[4] = { 'P', 'W', 'G', 'S' };
			uint32_t version = 1;
			int32_t worldSize = snapshot.worldSize;
			out.write(magic, 4);
			out.write(reinterpret_cast<const char*>(&version), sizeof(version));
			out.write(reinterpret_cast<const char*>(&worldSize), sizeof(worldSize));

			auto writeLayer = [&](const std::vector<float>& values)
			{
				uint32_t count = static_cast<uint32_t>(values.size());
				out.write(reinterpret_cast<const char*>(&count), sizeof(count));
				if (count > 0)
				{
					out.write(reinterpret_cast<const char*>(values.data()), sizeof(float) * count);
				}
			};

			writeLayer(snapshot.finalHeight);
			writeLayer(snapshot.water);
			writeLayer(snapshot.sediment);
			writeLayer(snapshot.deposit);
			writeLayer(snapshot.flowAccumulation);
			writeLayer(snapshot.riverChannels);
			return true;
		}

		bool ReadSimulationSidecar(const std::string& path, SimulationStateSnapshot& snapshot)
		{
			std::ifstream in(path, std::ios::binary);
			if (!in.is_open())
			{
				return false;
			}

			char magic[4] = { 0, 0, 0, 0 };
			in.read(magic, 4);
			if (in.gcount() != 4 || magic[0] != 'P' || magic[1] != 'W' || magic[2] != 'G' || magic[3] != 'S')
			{
				return false;
			}

			uint32_t version = 0;
			int32_t worldSize = 0;
			in.read(reinterpret_cast<char*>(&version), sizeof(version));
			in.read(reinterpret_cast<char*>(&worldSize), sizeof(worldSize));
			if (!in.good() || worldSize <= 0)
			{
				return false;
			}
			snapshot.worldSize = worldSize;

			auto readLayer = [&](std::vector<float>& values) -> bool
			{
				uint32_t count = 0;
				in.read(reinterpret_cast<char*>(&count), sizeof(count));
				if (!in.good())
				{
					return false;
				}
				values.resize(count);
				if (count > 0)
				{
					in.read(reinterpret_cast<char*>(values.data()), sizeof(float) * count);
				}
				return !in.fail();
			};

			if (!readLayer(snapshot.finalHeight))
			{
				return false;
			}
			readLayer(snapshot.water);
			readLayer(snapshot.sediment);
			readLayer(snapshot.deposit);
			readLayer(snapshot.flowAccumulation);
			readLayer(snapshot.riverChannels);
			return snapshot.HasData();
		}
	}

	bool ProjectSerializer::Save(EditorScene& scene, const std::string& path)
	{
		YAML::Node root;
		root["project"]["type"] = "PWG";
		root["project"]["version"] = 1;

		auto chunkManager = scene.GetTerrain()->GetChunkManager();
		root["terrain"]["chunkCount"] = chunkManager->GetChunkCount();
		root["terrain"]["chunkSize"] = chunkManager->GetChunkSize();

		auto pipeline = chunkManager->GetTerrainComputePipeline();
		root["hydraulic"] = WriteHydraulicSettings(pipeline->GetHydraulicSettings());
		if (scene.GetTerrainMaterialLibrary())
		{
			root["terrainMaterials"] = WriteTerrainMaterials(*scene.GetTerrainMaterialLibrary());
		}
		root["environment"]["skybox"] = WriteSkybox(scene.GetSkyboxDefinition());

		YAML::Node passes;
		for (const auto& pass : pipeline->GetGraph()->GetPasses())
		{
			YAML::Node passNode;
			for (const auto& param : pass->GetParameters())
			{
				WriteParameter(passNode, param);
			}
			passes[pass->GetName()] = passNode;
		}
		root["terrainGraph"] = passes;

		auto& light = scene.GetSunObject()->GetLight();
		auto& sun = scene.GetSunObject()->GetSunObjectProperties();
		root["light"]["position"] = Vec3Node(light.position);
		root["light"]["direction"] = Vec3Node(light.direction);
		root["light"]["color"] = Vec3Node(light.color);
		root["light"]["intensivity"] = light.intensivity;
		root["light"]["exposure"] = light.exposure;
		root["light"]["skyAmbient"] = light.skyAmbient;
		root["light"]["groundAmbient"] = light.groundAmbient;
		root["light"]["normalStrength"] = light.normalStrength;
		root["light"]["snowHeight"] = light.snowHeight;
		root["light"]["snowSoftness"] = light.snowSoftness;
		root["light"]["wetnessStrength"] = light.wetnessStrength;
		root["light"]["waterAlpha"] = light.waterAlpha;
		root["sun"]["orbitRadius"] = sun.orbitRadius;
		root["sun"]["orbitSpeed"] = sun.orbitSpeed;
		root["sun"]["time"] = sun.time;
		root["sun"]["color"] = Vec3Node(sun.color);
		root["sun"]["stopTime"] = sun.stopTime;

		root["camera"]["activeType"] = static_cast<int>(scene.GetCameraManager()->GetActiveCameraType());
		root["camera"]["position"] = Vec3Node(scene.GetCameraManager()->GetCameraPosition());
		root["editorUi"] = WriteEditorUi(scene);

		std::ofstream stream(path);
		if (!stream.is_open())
		{
			return false;
		}
		stream << root;
		stream.close();

		// Persist the live simulation textures (eroded height + water / rivers / lakes)
		// next to the project so reopening it brings the water back exactly as saved.
		SimulationStateSnapshot snapshot = chunkManager->CaptureSimulationState();
		WriteSimulationSidecar(SimulationSidecarPath(path), snapshot);
		return true;
	}

	bool ProjectSerializer::Load(EditorScene& scene, const std::string& path)
	{
		YAML::Node root = YAML::LoadFile(path);
		if (!root["project"] || root["project"]["type"].as<std::string>("") != "PWG")
		{
			return false;
		}

		auto chunkManager = scene.GetTerrain()->GetChunkManager();
		int chunkCount = root["terrain"]["chunkCount"] ? root["terrain"]["chunkCount"].as<int>() : chunkManager->GetChunkCount();
		int chunkSize = root["terrain"]["chunkSize"] ? root["terrain"]["chunkSize"].as<int>() : chunkManager->GetChunkSize();
		chunkManager->ConfigureTerrain(chunkCount, chunkSize);

		auto pipeline = chunkManager->GetTerrainComputePipeline();
		ReadHydraulicSettings(root["hydraulic"], pipeline->GetHydraulicSettings());
		if (scene.GetTerrainMaterialLibrary())
		{
			ReadTerrainMaterials(root["terrainMaterials"], *scene.GetTerrainMaterialLibrary());
		}
		ReadSkybox(root["environment"]["skybox"], scene);

		auto graphNode = root["terrainGraph"];
		if (graphNode)
		{
			for (const auto& pass : pipeline->GetGraph()->GetPasses())
			{
				auto passNode = graphNode[pass->GetName()];
				for (const auto& param : pass->GetParameters())
				{
					ReadParameter(passNode, param);
				}
			}
		}

		auto& light = scene.GetSunObject()->GetLight();
		auto lightNode = root["light"];
		if (lightNode)
		{
			light.position = ReadVec3(lightNode["position"], light.position);
			light.direction = ReadVec3(lightNode["direction"], light.direction);
			light.color = ReadVec3(lightNode["color"], light.color);
			ReadValue(lightNode, "intensivity", light.intensivity);
			ReadValue(lightNode, "exposure", light.exposure);
			ReadValue(lightNode, "skyAmbient", light.skyAmbient);
			ReadValue(lightNode, "groundAmbient", light.groundAmbient);
			ReadValue(lightNode, "normalStrength", light.normalStrength);
			ReadValue(lightNode, "snowHeight", light.snowHeight);
			ReadValue(lightNode, "snowSoftness", light.snowSoftness);
			ReadValue(lightNode, "wetnessStrength", light.wetnessStrength);
			ReadValue(lightNode, "waterAlpha", light.waterAlpha);
		}

		auto& sun = scene.GetSunObject()->GetSunObjectProperties();
		auto sunNode = root["sun"];
		if (sunNode)
		{
			ReadValue(sunNode, "orbitRadius", sun.orbitRadius);
			ReadValue(sunNode, "orbitSpeed", sun.orbitSpeed);
			ReadValue(sunNode, "time", sun.time);
			sun.color = ReadVec3(sunNode["color"], sun.color);
			ReadValue(sunNode, "stopTime", sun.stopTime);
		}

		auto cameraNode = root["camera"];
		if (cameraNode)
		{
			int cameraType = cameraNode["activeType"] ? cameraNode["activeType"].as<int>() : 0;
			scene.GetCameraManager()->SetCamera(static_cast<CameraType>(cameraType));
			glm::vec3 position = ReadVec3(cameraNode["position"], scene.GetCameraManager()->GetCameraPosition());
			scene.GetCameraManager()->SetCameraPosition(position.x, position.y, position.z);
		}
		ReadEditorUi(root["editorUi"], scene);

		chunkManager->GenerateWorld();

		// Bring back the saved simulation state (water + eroded terrain) if present.
		SimulationStateSnapshot snapshot;
		if (ReadSimulationSidecar(SimulationSidecarPath(path), snapshot))
		{
			chunkManager->RestoreSimulationState(snapshot);
		}
		return true;
	}
}
