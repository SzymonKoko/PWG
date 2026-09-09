#include "projectSerializer.h"

#include "scene/editorScene.h"
#include "scene/controls/terrainGraphEditor.h"
#include "scene/environment/environmentSettings.h"
#include "terrain/materials/terrainMaterialLibrary.h"
#include "terrain/graph/passes/terrainPass.h"
#include "terrain/graph/terrainPassContext.h"
#include "terrain/graph/universalTerrainGraph.h"
#include "terrain/chunk/chunkManager.h"
#include "yaml-cpp/yaml.h"

#include <fstream>
#include <unordered_map>
#include <filesystem>
#include <cstdint>
#include <cmath>
#include <algorithm>

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

		YAML::Node Vec2Node(const ImVec2& value)
		{
			YAML::Node node;
			node["x"] = value.x;
			node["y"] = value.y;
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

		YAML::Node WriteEnvironmentSettings(const EnvironmentSettings& settings)
		{
			YAML::Node node;
			node["timeOfDay"] = settings.timeOfDay;
			node["latitude"] = settings.latitude;
			node["animateTime"] = settings.animateTime;
			node["timeScale"] = settings.timeScale;
			node["sunAngularSize"] = settings.sunAngularSize;
			node["sunColor"] = Vec3Node(settings.sunColor);
			node["sunIlluminance"] = settings.sunIlluminance;
			node["skyTint"] = Vec3Node(settings.skyTint);
			node["atmosphereDensity"] = settings.atmosphereDensity;
			node["haze"] = settings.haze;
			node["groundAlbedo"] = Vec3Node(settings.groundAlbedo);
			node["useSkyboxBlend"] = settings.useSkyboxBlend;
			node["skyboxBlend"] = settings.skyboxBlend;
			node["exposure"] = settings.exposure;
			node["whitePoint"] = settings.whitePoint;
			node["bloomStrength"] = settings.bloomStrength;
			node["fogDensity"] = settings.fogDensity;
			node["bloomEnabled"] = settings.bloomEnabled;
			node["fxaaEnabled"] = settings.fxaaEnabled;
			node["shadowsEnabled"] = settings.shadowsEnabled;
			node["shadowResolution"] = settings.shadowResolution;
			node["shadowDistance"] = settings.shadowDistance;
			node["shadowStrength"] = settings.shadowStrength;
			node["shadowBias"] = settings.shadowBias;
			node["shadowNormalBias"] = settings.shadowNormalBias;
			node["skyAmbient"] = settings.skyAmbient;
			node["groundAmbient"] = settings.groundAmbient;
			node["normalStrength"] = settings.normalStrength;
			node["snowHeight"] = settings.snowHeight;
			node["snowSoftness"] = settings.snowSoftness;
			node["wetnessStrength"] = settings.wetnessStrength;
			node["waterAlpha"] = settings.waterAlpha;
			node["waterShallowColor"] = Vec3Node(settings.waterShallowColor);
			node["waterDeepColor"] = Vec3Node(settings.waterDeepColor);
			node["waterNormalStrength"] = settings.waterNormalStrength;
			node["waterWaveScale"] = settings.waterWaveScale;
			node["waterWaveSpeed"] = settings.waterWaveSpeed;
			node["waterRefractionStrength"] = settings.waterRefractionStrength;
			node["waterAbsorption"] = settings.waterAbsorption;
			node["waterFoamWidth"] = settings.waterFoamWidth;
			node["waterFoamIntensity"] = settings.waterFoamIntensity;
			node["waterRoughness"] = settings.waterRoughness;
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

		YAML::Node WriteCommonCameraSettings(const CameraProperties& settings)
		{
			YAML::Node node;
			node["fov"] = settings.fov;
			node["nearPlane"] = settings.nearPlane;
			node["farPlane"] = settings.farPlane;
			return node;
		}

		void ReadCommonCameraSettings(const YAML::Node& node, CameraProperties& settings)
		{
			ReadValue(node, "fov", settings.fov);
			ReadValue(node, "nearPlane", settings.nearPlane);
			ReadValue(node, "farPlane", settings.farPlane);
			settings.fov = std::clamp(settings.fov, 20.0f, 120.0f);
			settings.nearPlane = std::clamp(settings.nearPlane, 0.001f, 10.0f);
			settings.farPlane = std::max(settings.farPlane, settings.nearPlane + 1.0f);
		}

		YAML::Node WriteCameraSettings(CameraManager& manager)
		{
			YAML::Node node;
			auto editor = manager.GetEditorCamera();
			auto flyover = manager.GetFlyoverCamera();
			auto player = manager.GetPlayerCamera();

			node["editor"]["common"] = WriteCommonCameraSettings(editor->GetCameraProperties());
			auto& editorSettings = editor->GetProperties();
			node["editor"]["orbitSensitivity"] = editorSettings.orbitSensitivity;
			node["editor"]["panSpeed"] = editorSettings.panSpeed;
			node["editor"]["zoomSpeed"] = editorSettings.zoomSpeed;
			node["editor"]["minZoom"] = editorSettings.minZoom;
			node["editor"]["maxZoom"] = editorSettings.maxZoom;

			node["flyover"]["common"] = WriteCommonCameraSettings(flyover->GetCameraProperties());
			auto& flyoverSettings = flyover->GetProperties();
			node["flyover"]["sensitivity"] = flyoverSettings.sensitivity;
			node["flyover"]["acceleration"] = flyoverSettings.acceleration;
			node["flyover"]["flightDrag"] = flyoverSettings.flightDrag;
			node["flyover"]["braking"] = flyoverSettings.braking;
			node["flyover"]["maxSpeed"] = flyoverSettings.maxSpeed;
			node["flyover"]["boostMultiplier"] = flyoverSettings.boostMultiplier;
			node["flyover"]["lookSmoothing"] = flyoverSettings.lookSmoothing;
			node["flyover"]["gimbalStrengthDegrees"] = flyoverSettings.gimbalStrengthDegrees;
			node["flyover"]["gimbalSmoothing"] = flyoverSettings.gimbalSmoothing;

			node["player"]["common"] = WriteCommonCameraSettings(player->GetCameraProperties());
			auto& playerSettings = player->GetProperties();
			node["player"]["sensitivity"] = playerSettings.cameraSensitivity;
			node["player"]["acceleration"] = playerSettings.acceleration;
			node["player"]["maxGroundSpeed"] = playerSettings.maxGroundSpeed;
			node["player"]["groundDamping"] = playerSettings.groundDamping;
			node["player"]["airDamping"] = playerSettings.airDamping;
			node["player"]["gravity"] = playerSettings.gravity;
			node["player"]["jumpSpeed"] = playerSettings.jumpSpeed;
			node["player"]["eyeHeight"] = playerSettings.eyeHeight;
			node["player"]["groundSnapDistance"] = playerSettings.groundSnapDistance;
			node["player"]["headBobEnabled"] = playerSettings.headBobEnabled;
			node["player"]["headBobHeight"] = playerSettings.headBobHeight;
			node["player"]["headBobSide"] = playerSettings.headBobSide;
			node["player"]["headBobStrideLength"] = playerSettings.headBobStrideLength;
			node["player"]["headBobRollDegrees"] = playerSettings.headBobRollDegrees;
			node["player"]["headBobResponse"] = playerSettings.headBobResponse;
			return node;
		}

		void ReadCameraSettings(const YAML::Node& node, CameraManager& manager)
		{
			if (!node) return;
			auto editor = manager.GetEditorCamera();
			auto flyover = manager.GetFlyoverCamera();
			auto player = manager.GetPlayerCamera();

			auto editorNode = node["editor"];
			ReadCommonCameraSettings(editorNode["common"], editor->GetCameraProperties());
			auto& editorSettings = editor->GetProperties();
			ReadValue(editorNode, "orbitSensitivity", editorSettings.orbitSensitivity);
			ReadValue(editorNode, "panSpeed", editorSettings.panSpeed);
			ReadValue(editorNode, "zoomSpeed", editorSettings.zoomSpeed);
			ReadValue(editorNode, "minZoom", editorSettings.minZoom);
			ReadValue(editorNode, "maxZoom", editorSettings.maxZoom);

			auto flyoverNode = node["flyover"];
			ReadCommonCameraSettings(flyoverNode["common"], flyover->GetCameraProperties());
			auto& flyoverSettings = flyover->GetProperties();
			ReadValue(flyoverNode, "sensitivity", flyoverSettings.sensitivity);
			ReadValue(flyoverNode, "acceleration", flyoverSettings.acceleration);
			ReadValue(flyoverNode, "flightDrag", flyoverSettings.flightDrag);
			ReadValue(flyoverNode, "braking", flyoverSettings.braking);
			ReadValue(flyoverNode, "maxSpeed", flyoverSettings.maxSpeed);
			ReadValue(flyoverNode, "boostMultiplier", flyoverSettings.boostMultiplier);
			ReadValue(flyoverNode, "lookSmoothing", flyoverSettings.lookSmoothing);
			ReadValue(flyoverNode, "gimbalStrengthDegrees", flyoverSettings.gimbalStrengthDegrees);
			ReadValue(flyoverNode, "gimbalSmoothing", flyoverSettings.gimbalSmoothing);

			auto playerNode = node["player"];
			ReadCommonCameraSettings(playerNode["common"], player->GetCameraProperties());
			auto& playerSettings = player->GetProperties();
			ReadValue(playerNode, "sensitivity", playerSettings.cameraSensitivity);
			ReadValue(playerNode, "acceleration", playerSettings.acceleration);
			ReadValue(playerNode, "maxGroundSpeed", playerSettings.maxGroundSpeed);
			ReadValue(playerNode, "groundDamping", playerSettings.groundDamping);
			ReadValue(playerNode, "airDamping", playerSettings.airDamping);
			ReadValue(playerNode, "gravity", playerSettings.gravity);
			ReadValue(playerNode, "jumpSpeed", playerSettings.jumpSpeed);
			ReadValue(playerNode, "eyeHeight", playerSettings.eyeHeight);
			ReadValue(playerNode, "groundSnapDistance", playerSettings.groundSnapDistance);
			ReadValue(playerNode, "headBobEnabled", playerSettings.headBobEnabled);
			ReadValue(playerNode, "headBobHeight", playerSettings.headBobHeight);
			ReadValue(playerNode, "headBobSide", playerSettings.headBobSide);
			ReadValue(playerNode, "headBobStrideLength", playerSettings.headBobStrideLength);
			ReadValue(playerNode, "headBobRollDegrees", playerSettings.headBobRollDegrees);
			ReadValue(playerNode, "headBobResponse", playerSettings.headBobResponse);
		}

		YAML::Node WriteEditorUi(EditorScene& scene)
		{
			YAML::Node node;
			node["showMainView"] = true;
			node["showTerrainControls"] = scene.ShowTerrainControls();
			node["showTerrainGraph"] = scene.ShowTerrainGraph();
			node["showWorld"] = scene.ShowWorld();
			node["showOutliner"] = scene.ShowOutliner();
			node["showDebugWindow"] = scene.ShowDebugWindow();
			node["showMaterialLibrary"] = scene.ShowMaterialLibrary();
			node["showCameraSettings"] = scene.ShowCameraSettings();
			node["imguiLayout"] = scene.GetProjectLayoutIni();

			const auto& graphView = scene.GetTerrainGraphViewState();
			YAML::Node graphViewNode;
			graphViewNode["pan"] = Vec2Node(graphView.pan);
			graphViewNode["zoom"] = graphView.zoom;
			for (const auto& graphNode : graphView.nodes)
			{
				YAML::Node nodeLayout;
				nodeLayout["pass"] = graphNode.passName;
				nodeLayout["position"] = Vec2Node(graphNode.position);
				graphViewNode["nodes"].push_back(nodeLayout);
			}
			for (const auto& graphEdge : graphView.proposedEdges)
			{
				YAML::Node edgeNode;
				edgeNode["fromPass"] = graphEdge.fromPass;
				edgeNode["toPass"] = graphEdge.toPass;
				edgeNode["maskName"] = graphEdge.maskName;
				graphViewNode["proposedEdges"].push_back(edgeNode);
			}
			node["terrainGraphView"] = graphViewNode;
			return node;
		}

		ImVec2 ReadVec2(const YAML::Node& node, const ImVec2& fallback)
		{
			if (!node)
			{
				return fallback;
			}
			return {
				node["x"] ? node["x"].as<float>() : fallback.x,
				node["y"] ? node["y"].as<float>() : fallback.y
			};
		}

		void ReadEditorUi(const YAML::Node& node, EditorScene& scene)
		{
			if (!node)
			{
				return;
			}

			scene.ShowMainView() = true;
			ReadValue(node, "showTerrainControls", scene.ShowTerrainControls());
			ReadValue(node, "showTerrainGraph", scene.ShowTerrainGraph());
			ReadValue(node, "showWorld", scene.ShowWorld());
			ReadValue(node, "showOutliner", scene.ShowOutliner());
			ReadValue(node, "showDebugWindow", scene.ShowDebugWindow());
			ReadValue(node, "showMaterialLibrary", scene.ShowMaterialLibrary());
			ReadValue(node, "showCameraSettings", scene.ShowCameraSettings());
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

			auto graphViewNode = node["terrainGraphView"];
			if (graphViewNode)
			{
				auto graphView = scene.GetTerrainGraphViewState();
				graphView.pan = ReadVec2(graphViewNode["pan"], graphView.pan);
				ReadValue(graphViewNode, "zoom", graphView.zoom);
				graphView.zoom = std::clamp(graphView.zoom, 0.30f, 2.75f);
				graphView.nodes.clear();
				graphView.proposedEdges.clear();

				auto nodes = graphViewNode["nodes"];
				if (nodes && nodes.IsSequence())
				{
					for (const auto& nodeLayout : nodes)
					{
						if (!nodeLayout["pass"])
						{
							continue;
						}
						controls::TerrainGraphNodeLayout layoutNode;
						layoutNode.passName = nodeLayout["pass"].as<std::string>();
						layoutNode.position = ReadVec2(nodeLayout["position"], layoutNode.position);
						graphView.nodes.push_back(layoutNode);
					}
				}

				auto proposedEdges = graphViewNode["proposedEdges"];
				if (proposedEdges && proposedEdges.IsSequence())
				{
					for (const auto& edgeNode : proposedEdges)
					{
						if (!edgeNode["fromPass"] || !edgeNode["toPass"] || !edgeNode["maskName"])
						{
							continue;
						}
						controls::TerrainGraphProposedEdge edge;
						edge.fromPass = edgeNode["fromPass"].as<std::string>();
						edge.toPass = edgeNode["toPass"].as<std::string>();
						edge.maskName = edgeNode["maskName"].as<std::string>();
						graphView.proposedEdges.push_back(edge);
					}
				}
				scene.SetTerrainGraphViewState(graphView);
			}
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

		void ReadEnvironmentSettings(const YAML::Node& node, EnvironmentSettings& settings)
		{
			if (!node)
			{
				return;
			}

			ReadValue(node, "timeOfDay", settings.timeOfDay);
			ReadValue(node, "latitude", settings.latitude);
			ReadValue(node, "animateTime", settings.animateTime);
			ReadValue(node, "timeScale", settings.timeScale);
			ReadValue(node, "sunAngularSize", settings.sunAngularSize);
			settings.sunColor = ReadVec3(node["sunColor"], settings.sunColor);
			ReadValue(node, "sunIlluminance", settings.sunIlluminance);
			settings.skyTint = ReadVec3(node["skyTint"], settings.skyTint);
			ReadValue(node, "atmosphereDensity", settings.atmosphereDensity);
			ReadValue(node, "haze", settings.haze);
			settings.groundAlbedo = ReadVec3(node["groundAlbedo"], settings.groundAlbedo);
			ReadValue(node, "useSkyboxBlend", settings.useSkyboxBlend);
			ReadValue(node, "skyboxBlend", settings.skyboxBlend);
			ReadValue(node, "exposure", settings.exposure);
			ReadValue(node, "whitePoint", settings.whitePoint);
			ReadValue(node, "bloomStrength", settings.bloomStrength);
			ReadValue(node, "fogDensity", settings.fogDensity);
			ReadValue(node, "bloomEnabled", settings.bloomEnabled);
			ReadValue(node, "fxaaEnabled", settings.fxaaEnabled);
			ReadValue(node, "shadowsEnabled", settings.shadowsEnabled);
			ReadValue(node, "shadowResolution", settings.shadowResolution);
			ReadValue(node, "shadowDistance", settings.shadowDistance);
			ReadValue(node, "shadowStrength", settings.shadowStrength);
			ReadValue(node, "shadowBias", settings.shadowBias);
			ReadValue(node, "shadowNormalBias", settings.shadowNormalBias);
			ReadValue(node, "skyAmbient", settings.skyAmbient);
			ReadValue(node, "groundAmbient", settings.groundAmbient);
			ReadValue(node, "normalStrength", settings.normalStrength);
			ReadValue(node, "snowHeight", settings.snowHeight);
			ReadValue(node, "snowSoftness", settings.snowSoftness);
			ReadValue(node, "wetnessStrength", settings.wetnessStrength);
			ReadValue(node, "waterAlpha", settings.waterAlpha);
			settings.waterShallowColor = ReadVec3(node["waterShallowColor"], settings.waterShallowColor);
			settings.waterDeepColor = ReadVec3(node["waterDeepColor"], settings.waterDeepColor);
			ReadValue(node, "waterNormalStrength", settings.waterNormalStrength);
			ReadValue(node, "waterWaveScale", settings.waterWaveScale);
			ReadValue(node, "waterWaveSpeed", settings.waterWaveSpeed);
			ReadValue(node, "waterRefractionStrength", settings.waterRefractionStrength);
			ReadValue(node, "waterAbsorption", settings.waterAbsorption);
			ReadValue(node, "waterFoamWidth", settings.waterFoamWidth);
			ReadValue(node, "waterFoamIntensity", settings.waterFoamIntensity);
			ReadValue(node, "waterRoughness", settings.waterRoughness);
		}

		void ReadLegacyLighting(const YAML::Node& lightNode, const YAML::Node& sunNode, EnvironmentSettings& settings)
		{
			if (lightNode)
			{
				settings.sunColor = ReadVec3(lightNode["color"], settings.sunColor);
				ReadValue(lightNode, "intensivity", settings.sunIlluminance);
				ReadValue(lightNode, "exposure", settings.exposure);
				ReadValue(lightNode, "skyAmbient", settings.skyAmbient);
				ReadValue(lightNode, "groundAmbient", settings.groundAmbient);
				ReadValue(lightNode, "normalStrength", settings.normalStrength);
				ReadValue(lightNode, "snowHeight", settings.snowHeight);
				ReadValue(lightNode, "snowSoftness", settings.snowSoftness);
				ReadValue(lightNode, "wetnessStrength", settings.wetnessStrength);
				ReadValue(lightNode, "waterAlpha", settings.waterAlpha);
			}
			if (sunNode)
			{
				float legacyTime = settings.timeOfDay;
				ReadValue(sunNode, "time", legacyTime);
				settings.timeOfDay = std::fmod(std::abs(legacyTime), 360.0f) / 360.0f * 24.0f;
				bool stopTime = true;
				ReadValue(sunNode, "stopTime", stopTime);
				settings.animateTime = !stopTime;
				ReadValue(sunNode, "orbitSpeed", settings.timeScale);
			}
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

		YAML::Node WriteGraphParameter(const TerrainGraphParameter& param)
		{
			YAML::Node node;
			node["id"] = param.id;
			node["type"] = static_cast<int>(param.type);
			node["float"] = param.floatValue;
			node["min"] = param.minValue;
			node["max"] = param.maxValue;
			node["int"] = param.intValue;
			node["bool"] = param.boolValue;
			node["string"] = param.stringValue;
			return node;
		}

		void ReadGraphParameter(const YAML::Node& node, TerrainGraphParameter& param)
		{
			if (!node)
			{
				return;
			}
			if (node["float"]) param.floatValue = node["float"].as<float>();
			if (node["min"]) param.minValue = node["min"].as<float>();
			if (node["max"]) param.maxValue = node["max"].as<float>();
			if (node["int"]) param.intValue = node["int"].as<int>();
			if (node["bool"]) param.boolValue = node["bool"].as<bool>();
			if (node["string"]) param.stringValue = node["string"].as<std::string>();
		}

		void MigrateVegetationDefaults(TerrainGraphNodeInstance& node)
		{
			auto parameter = [&](const char* id) -> TerrainGraphParameter*
			{
				auto found = std::find_if(node.parameters.begin(), node.parameters.end(), [&](const TerrainGraphParameter& value)
				{
					return value.id == id;
				});
				return found != node.parameters.end() ? &(*found) : nullptr;
			};
			if (node.typeId == "grassAsset")
			{
				auto* density = parameter("density");
				auto* spacing = parameter("spacing");
				auto* renderDistance = parameter("renderDistance");
				if (density && spacing && renderDistance &&
				std::abs(density->floatValue - 0.65f) < 0.0001f &&
				std::abs(spacing->floatValue - 5.0f) < 0.0001f &&
				std::abs(renderDistance->floatValue - 220.0f) < 0.0001f)
				{
					density->floatValue = 0.95f;
					spacing->floatValue = 3.0f;
					renderDistance->floatValue = 180.0f;
				}
			}
			else if (node.typeId == "treeAssetA" || node.typeId == "treeAssetB")
			{
				auto* maxSlope = parameter("maxSlope");
				if (maxSlope && std::abs(maxSlope->floatValue - 28.0f) < 0.0001f)
				{
					maxSlope->floatValue = 15.0f;
				}
			}
		}

		YAML::Node WriteUniversalTerrainGraph(const TerrainGraphAsset& asset)
		{
			YAML::Node node;
			node["useUniversalGraph"] = asset.useUniversalGraph;
			node["pan"] = Vec2Node(asset.pan);
			node["zoom"] = asset.zoom;
			node["nextNodeId"] = asset.nextNodeId;
			node["nextLinkId"] = asset.nextLinkId;
			for (const auto& graphNode : asset.nodes)
			{
				YAML::Node nodeData;
				nodeData["id"] = graphNode.id;
				nodeData["typeId"] = graphNode.typeId;
				nodeData["displayName"] = graphNode.displayName;
				nodeData["enabled"] = graphNode.enabled;
				nodeData["position"] = Vec2Node(graphNode.position);
				for (const auto& param : graphNode.parameters)
				{
					nodeData["parameters"].push_back(WriteGraphParameter(param));
				}
				node["nodes"].push_back(nodeData);
			}
			for (const auto& link : asset.links)
			{
				YAML::Node linkData;
				linkData["id"] = link.id;
				linkData["fromNodeId"] = link.fromNodeId;
				linkData["fromPortId"] = link.fromPortId;
				linkData["toNodeId"] = link.toNodeId;
				linkData["toPortId"] = link.toPortId;
				node["links"].push_back(linkData);
			}
			return node;
		}

		void ReadUniversalTerrainGraph(const YAML::Node& node, TerrainGraphAsset& asset)
		{
			if (!node)
			{
				return;
			}

			asset.Clear();
			asset.useUniversalGraph = node["useUniversalGraph"] ? node["useUniversalGraph"].as<bool>() : false;
			asset.pan = ReadVec2(node["pan"], asset.pan);
			if (node["zoom"]) asset.zoom = std::clamp(node["zoom"].as<float>(), 0.30f, 2.75f);
			if (node["nextNodeId"]) asset.nextNodeId = node["nextNodeId"].as<int>();
			if (node["nextLinkId"]) asset.nextLinkId = node["nextLinkId"].as<int>();

			auto nodes = node["nodes"];
			if (nodes && nodes.IsSequence())
			{
				for (const auto& nodeData : nodes)
				{
					if (!nodeData["id"] || !nodeData["typeId"])
					{
						continue;
					}
					const auto* type = FindTerrainGraphNodeType(nodeData["typeId"].as<std::string>());
					if (!type)
					{
						continue;
					}
					TerrainGraphNodeInstance graphNode;
					graphNode.id = nodeData["id"].as<int>();
					graphNode.typeId = type->typeId;
					graphNode.displayName = nodeData["displayName"] ? nodeData["displayName"].as<std::string>() : type->displayName;
					graphNode.enabled = nodeData["enabled"] ? nodeData["enabled"].as<bool>() : true;
					graphNode.position = ReadVec2(nodeData["position"], graphNode.position);
					graphNode.parameters = type->parameters;
					auto params = nodeData["parameters"];
					if (params && params.IsSequence())
					{
						for (const auto& paramNode : params)
						{
							if (!paramNode["id"])
							{
								continue;
							}
							std::string id = paramNode["id"].as<std::string>();
							auto paramIt = std::find_if(graphNode.parameters.begin(), graphNode.parameters.end(), [&](const TerrainGraphParameter& param)
							{
								return param.id == id;
							});
							if (paramIt != graphNode.parameters.end())
							{
								ReadGraphParameter(paramNode, *paramIt);
							}
						}
					}
					MigrateVegetationDefaults(graphNode);
					asset.nodes.push_back(std::move(graphNode));
				}
			}

			auto links = node["links"];
			if (links && links.IsSequence())
			{
				for (const auto& linkData : links)
				{
					if (!linkData["fromNodeId"] || !linkData["fromPortId"] || !linkData["toNodeId"] || !linkData["toPortId"])
					{
						continue;
					}
					TerrainGraphLink link;
					link.id = linkData["id"] ? linkData["id"].as<int>() : asset.nextLinkId++;
					link.fromNodeId = linkData["fromNodeId"].as<int>();
					link.fromPortId = linkData["fromPortId"].as<std::string>();
					link.toNodeId = linkData["toNodeId"].as<int>();
					link.toPortId = linkData["toPortId"].as<std::string>();
					asset.links.push_back(std::move(link));
				}
			}

			for (const auto& graphNode : asset.nodes)
			{
				asset.nextNodeId = std::max(asset.nextNodeId, graphNode.id + 1);
			}
			for (const auto& link : asset.links)
			{
				asset.nextLinkId = std::max(asset.nextLinkId, link.id + 1);
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
			node["minVisibleWaterDepth"] = settings.minVisibleWaterDepth;
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
			ReadValue(node, "minVisibleWaterDepth", settings.minVisibleWaterDepth);
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
		root["environment"]["settings"] = WriteEnvironmentSettings(scene.GetEnvironmentSystem()->GetSettings());

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
		root["terrainGraphV2"] = WriteUniversalTerrainGraph(pipeline->GetUniversalGraphAsset());

		root["camera"]["activeType"] = static_cast<int>(scene.GetCameraManager()->GetActiveCameraType());
		root["camera"]["position"] = Vec3Node(scene.GetCameraManager()->GetCameraPosition());
		root["camera"]["settings"] = WriteCameraSettings(*scene.GetCameraManager());
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
		auto& environmentSettings = scene.GetEnvironmentSystem()->GetSettings();
		if (root["environment"]["settings"])
		{
			ReadEnvironmentSettings(root["environment"]["settings"], environmentSettings);
		}
		else
		{
			ReadLegacyLighting(root["light"], root["sun"], environmentSettings);
		}

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
		const auto universalGraphNode = root["terrainGraphV2"];
		ReadUniversalTerrainGraph(universalGraphNode, pipeline->GetUniversalGraphAsset());
		controls::TerrainGraphEditor::ResetHistory();
		scene.ReloadVegetationAssets();

		auto cameraNode = root["camera"];
		if (cameraNode)
		{
			ReadCameraSettings(cameraNode["settings"], *scene.GetCameraManager());
			int cameraType = cameraNode["activeType"] ? cameraNode["activeType"].as<int>() : 0;
			scene.GetCameraManager()->SetCamera(static_cast<CameraType>(cameraType));
			glm::vec3 position = ReadVec3(cameraNode["position"], scene.GetCameraManager()->GetCameraPosition());
			scene.GetCameraManager()->SetCameraPosition(position.x, position.y, position.z);
		}
		ReadEditorUi(root["editorUi"], scene);
		chunkManager->GenerateWorld();
		if (universalGraphNode && !pipeline->GetUniversalGraphAsset().nodes.empty())
		{
			chunkManager->RunUniversalGraph();
		}

		// Bring back the saved simulation state (water + eroded terrain) if present.
		SimulationStateSnapshot snapshot;
		if (ReadSimulationSidecar(SimulationSidecarPath(path), snapshot))
		{
			chunkManager->RestoreSimulationState(snapshot);
		}
		return true;
	}
}



