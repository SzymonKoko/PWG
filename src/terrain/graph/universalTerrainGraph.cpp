#include "universalTerrainGraph.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace pwg
{
	namespace
	{
		TerrainGraphParameter FloatParam(std::string id, std::string label, float value, float minValue, float maxValue)
		{
			TerrainGraphParameter param;
			param.id = std::move(id);
			param.displayName = std::move(label);
			param.type = TerrainGraphParameterType::Float;
			param.floatValue = value;
			param.minValue = minValue;
			param.maxValue = maxValue;
			return param;
		}

		TerrainGraphParameter IntParam(std::string id, std::string label, int value, float minValue, float maxValue)
		{
			TerrainGraphParameter param;
			param.id = std::move(id);
			param.displayName = std::move(label);
			param.type = TerrainGraphParameterType::Int;
			param.intValue = value;
			param.minValue = minValue;
			param.maxValue = maxValue;
			return param;
		}

		TerrainGraphParameter BoolParam(std::string id, std::string label, bool value)
		{
			TerrainGraphParameter param;
			param.id = std::move(id);
			param.displayName = std::move(label);
			param.type = TerrainGraphParameterType::Bool;
			param.boolValue = value;
			return param;
		}

		TerrainGraphParameter StringParam(std::string id, std::string label, std::string value)
		{
			TerrainGraphParameter param;
			param.id = std::move(id);
			param.displayName = std::move(label);
			param.type = TerrainGraphParameterType::String;
			param.stringValue = std::move(value);
			return param;
		}

		TerrainGraphPortDefinition Input(std::string id, std::string label, bool required = true, float defaultValue = 0.0f)
		{
			return { std::move(id), std::move(label), TerrainGraphValueType::FloatMask, required, defaultValue };
		}

		TerrainGraphPortDefinition Output(std::string id, std::string label)
		{
			return { std::move(id), std::move(label), TerrainGraphValueType::FloatMask, false, 0.0f };
		}

		const TerrainGraphParameter* FindParameter(const TerrainGraphNodeInstance& node, const std::string& id)
		{
			auto it = std::find_if(node.parameters.begin(), node.parameters.end(), [&](const TerrainGraphParameter& param)
			{
				return param.id == id;
			});
			return it != node.parameters.end() ? &(*it) : nullptr;
		}

		bool LinkMatches(const TerrainGraphLink& link, int fromNodeId, const std::string& fromPortId, int toNodeId, const std::string& toPortId)
		{
			return link.fromNodeId == fromNodeId && link.fromPortId == fromPortId && link.toNodeId == toNodeId && link.toPortId == toPortId;
		}
	}

	TerrainGraphNodeInstance* TerrainGraphAsset::FindNode(int nodeId)
	{
		auto it = std::find_if(nodes.begin(), nodes.end(), [&](const TerrainGraphNodeInstance& node)
		{
			return node.id == nodeId;
		});
		return it != nodes.end() ? &(*it) : nullptr;
	}

	const TerrainGraphNodeInstance* TerrainGraphAsset::FindNode(int nodeId) const
	{
		auto it = std::find_if(nodes.begin(), nodes.end(), [&](const TerrainGraphNodeInstance& node)
		{
			return node.id == nodeId;
		});
		return it != nodes.end() ? &(*it) : nullptr;
	}

	const TerrainGraphLink* TerrainGraphAsset::FindInputLink(int nodeId, const std::string& portId) const
	{
		auto it = std::find_if(links.begin(), links.end(), [&](const TerrainGraphLink& link)
		{
			return link.toNodeId == nodeId && link.toPortId == portId;
		});
		return it != links.end() ? &(*it) : nullptr;
	}

	TerrainGraphNodeInstance& TerrainGraphAsset::AddNode(const TerrainGraphNodeType& type, ImVec2 position)
	{
		TerrainGraphNodeInstance node;
		node.id = nextNodeId++;
		node.typeId = type.typeId;
		node.displayName = type.displayName;
		node.position = position;
		node.parameters = type.parameters;
		nodes.push_back(std::move(node));
		return nodes.back();
	}

	bool TerrainGraphAsset::RemoveNode(int nodeId)
	{
		auto beforeNodes = nodes.size();
		nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [&](const TerrainGraphNodeInstance& node)
		{
			return node.id == nodeId;
		}), nodes.end());
		links.erase(std::remove_if(links.begin(), links.end(), [&](const TerrainGraphLink& link)
		{
			return link.fromNodeId == nodeId || link.toNodeId == nodeId;
		}), links.end());
		return nodes.size() != beforeNodes;
	}

	bool TerrainGraphAsset::AddLink(int fromNodeId, const std::string& fromPortId, int toNodeId, const std::string& toPortId)
	{
		if (fromNodeId == toNodeId || !FindNode(fromNodeId) || !FindNode(toNodeId))
		{
			return false;
		}
		if (std::any_of(links.begin(), links.end(), [&](const TerrainGraphLink& link)
			{
				return LinkMatches(link, fromNodeId, fromPortId, toNodeId, toPortId);
			}))
		{
			return false;
		}
		links.erase(std::remove_if(links.begin(), links.end(), [&](const TerrainGraphLink& link)
		{
			return link.toNodeId == toNodeId && link.toPortId == toPortId;
		}), links.end());

		TerrainGraphLink link;
		link.id = nextLinkId++;
		link.fromNodeId = fromNodeId;
		link.fromPortId = fromPortId;
		link.toNodeId = toNodeId;
		link.toPortId = toPortId;
		links.push_back(std::move(link));
		return true;
	}

	bool TerrainGraphAsset::RemoveLink(int linkId)
	{
		auto before = links.size();
		links.erase(std::remove_if(links.begin(), links.end(), [&](const TerrainGraphLink& link)
		{
			return link.id == linkId;
		}), links.end());
		return links.size() != before;
	}

	void TerrainGraphAsset::Clear()
	{
		nodes.clear();
		links.clear();
		nextNodeId = 1;
		nextLinkId = 1;
	}

	const std::vector<TerrainGraphNodeType>& GetTerrainGraphNodeTypes()
	{
		static const std::vector<TerrainGraphNodeType> types =
		{
			{ "constant", "Constant", "Generators", {}, { Output("value", "Value") }, { FloatParam("value", "Value", 100.0f, -20000.0f, 20000.0f) } },
			{ "noiseHeight", "Noise Height", "Generators", {}, { Output("height", "Height") }, {
				IntParam("noiseType", "Noise Type", 0, 0.0f, 5.0f),
				IntParam("fractalType", "Fractal Type", 1, 0.0f, 3.0f),
				IntParam("seed", "Seed", 42, 1.0f, 100000.0f),
				FloatParam("frequency", "Frequency", 1.0f, 0.01f, 8.0f),
				FloatParam("scale", "Scale", 128.0f, 1.0f, 512.0f),
				IntParam("octaves", "Octaves", 4, 1.0f, 12.0f),
				FloatParam("lacunarity", "Lacunarity", 2.0f, 0.1f, 5.0f),
				FloatParam("gain", "Gain / Persistence", 0.5f, 0.0f, 1.0f),
				FloatParam("weightedStrength", "Weighted Strength", 0.0f, 0.0f, 1.0f),
				FloatParam("pingPongStrength", "Ping Pong Strength", 2.0f, 0.1f, 8.0f)
			} },
			{ "noiseMask", "Noise Mask", "Generators", {}, { Output("mask", "Mask") }, {
				IntParam("noiseType", "Noise Type", 0, 0.0f, 5.0f),
				IntParam("fractalType", "Fractal Type", 1, 0.0f, 3.0f),
				IntParam("seed", "Seed", 1337, 1.0f, 100000.0f),
				FloatParam("frequency", "Frequency", 1.0f, 0.01f, 8.0f),
				FloatParam("scale", "Scale", 128.0f, 1.0f, 512.0f),
				IntParam("octaves", "Octaves", 4, 1.0f, 12.0f),
				FloatParam("lacunarity", "Lacunarity", 2.0f, 0.1f, 5.0f),
				FloatParam("gain", "Gain / Persistence", 0.5f, 0.0f, 1.0f),
				FloatParam("weightedStrength", "Weighted Strength", 0.0f, 0.0f, 1.0f),
				FloatParam("pingPongStrength", "Ping Pong Strength", 2.0f, 0.1f, 8.0f)
			} },
			{ "add", "Add", "Math", { Input("a", "A"), Input("b", "B") }, { Output("result", "Result") }, {} },
			{ "subtract", "Subtract", "Math", { Input("a", "A"), Input("b", "B") }, { Output("result", "Result") }, {} },
			{ "multiply", "Multiply", "Math", { Input("a", "A"), Input("b", "B") }, { Output("result", "Result") }, {} },
			{ "divide", "Divide", "Math", { Input("a", "A"), Input("b", "B") }, { Output("result", "Result") }, {} },
			{ "min", "Min", "Math", { Input("a", "A"), Input("b", "B") }, { Output("result", "Result") }, {} },
			{ "max", "Max", "Math", { Input("a", "A"), Input("b", "B") }, { Output("result", "Result") }, {} },
			{ "lerp", "Lerp", "Math", { Input("a", "A"), Input("b", "B"), Input("t", "T", false, 0.5f) }, { Output("result", "Result") }, {} },
			{ "clamp", "Clamp", "Math", { Input("value", "Value") }, { Output("result", "Result") }, {
				FloatParam("min", "Min", 0.0f, -1.0f, 1.0f),
				FloatParam("max", "Max", 1.0f, -1.0f, 2.0f)
			} },
			{ "normalize", "Normalize", "Math", { Input("value", "Value") }, { Output("result", "Result") }, {} },
			{ "abs", "Abs", "Math", { Input("value", "Value") }, { Output("result", "Result") }, {} },
			{ "invert", "Invert", "Math", { Input("value", "Value") }, { Output("result", "Result") }, {
				FloatParam("pivot", "Pivot", 1.0f, -20000.0f, 20000.0f)
			} },
			{ "fitRange", "Fit Range", "Math", { Input("value", "Value") }, { Output("result", "Result") }, {
				FloatParam("inMin", "Input Min", 0.0f, -20000.0f, 20000.0f),
				FloatParam("inMax", "Input Max", 1.0f, -20000.0f, 20000.0f),
				FloatParam("outMin", "Output Min", 0.0f, -20000.0f, 20000.0f),
				FloatParam("outMax", "Output Max", 1.0f, -20000.0f, 20000.0f)
			} },
			{ "smoothstep", "Smoothstep", "Math", { Input("value", "Value") }, { Output("result", "Result") }, {
				FloatParam("min", "Edge Min", 0.25f, -20000.0f, 20000.0f),
				FloatParam("max", "Edge Max", 0.75f, -20000.0f, 20000.0f)
			} },
			{ "power", "Power", "Math", { Input("value", "Value") }, { Output("result", "Result") }, {
				FloatParam("power", "Power", 1.5f, 0.05f, 8.0f)
			} },
			{ "thresholdMask", "Threshold Mask", "Terrain", { Input("height", "Height") }, { Output("mask", "Mask") }, {
				FloatParam("min", "Min", 0.35f, 0.0f, 1.0f),
				FloatParam("max", "Max", 1.0f, 0.0f, 1.0f)
			} },
			{ "heightBandMask", "Height Band Mask", "Terrain", { Input("height", "Height") }, { Output("mask", "Mask") }, {
				FloatParam("min", "Min Height", 0.0f, -20000.0f, 20000.0f),
				FloatParam("max", "Max Height", 400.0f, -20000.0f, 20000.0f)
			} },
			{ "slopeMask", "Slope Mask", "Terrain", { Input("height", "Height") }, { Output("mask", "Mask") }, {
				FloatParam("scale", "Sensitivity", 0.05f, 0.0001f, 2.0f),
				FloatParam("min", "Min Slope", 0.0f, 0.0f, 1.0f),
				FloatParam("max", "Max Slope", 1.0f, 0.0f, 1.0f)
			} },
			{ "curvatureMask", "Curvature Mask", "Terrain", { Input("height", "Height") }, { Output("mask", "Mask") }, {
				IntParam("mode", "Mode", 0, 0.0f, 2.0f),
				FloatParam("scale", "Sensitivity", 0.02f, 0.0001f, 2.0f),
				FloatParam("bias", "Bias", 0.5f, 0.0f, 1.0f)
			} },
			{ "blurMask", "Blur Mask", "Terrain", { Input("mask", "Mask") }, { Output("mask", "Mask") }, {
				IntParam("radius", "Radius", 2, 1.0f, 8.0f)
			} },
			{ "erodeMask", "Erode Mask", "Terrain", { Input("mask", "Mask") }, { Output("mask", "Mask") }, {
				IntParam("radius", "Radius", 2, 1.0f, 8.0f)
			} },
			{ "dilateMask", "Dilate Mask", "Terrain", { Input("mask", "Mask") }, { Output("mask", "Mask") }, {
				IntParam("radius", "Radius", 2, 1.0f, 8.0f)
			} },
			{ "terrace", "Terrace", "Terrain", { Input("height", "Height") }, { Output("height", "Height") }, {
				FloatParam("min", "Min Height", 0.0f, -20000.0f, 20000.0f),
				FloatParam("max", "Max Height", 1.0f, -20000.0f, 20000.0f),
				IntParam("steps", "Steps", 8, 2.0f, 64.0f)
			} },
			{ "posterizeMask", "Posterize Mask", "Terrain", { Input("mask", "Mask") }, { Output("mask", "Mask") }, {
				IntParam("steps", "Steps", 4, 2.0f, 64.0f)
			} },
			{ "drainageNetwork", "Drainage Network", "Simulation", {}, {
				Output("water", "Water"), Output("flow", "Flow"),
				Output("rivers", "Rivers"), Output("lakes", "Lakes")
			}, {
				FloatParam("riverAmount", "River Amount", 0.5f, 0.0f, 1.0f),
				FloatParam("lakeAmount", "Lake Amount", 0.5f, 0.0f, 1.0f),
				IntParam("seed", "Seed", 1337, 1.0f, 100000.0f)
			} },
			{ "particleErosion", "Particle Erosion", "Terrain", { Input("height", "Height") }, {
				Output("height", "Height"),
				Output("flow", "Flow"),
				Output("deposition", "Deposition"),
				Output("erosion", "Erosion")
			}, {
				FloatParam("amount", "Amount", 0.35f, 0.0f, 1.0f),
				IntParam("seed", "Seed", 1337, 1.0f, 100000.0f)
			} },
			{ "thermalErosion", "Thermal Erosion", "Terrain", {
				Input("height", "Height"), Input("hardness", "Hardness", false, 0.5f)
			}, {
				Output("height", "Height"), Output("debris", "Debris"),
				Output("erosion", "Erosion"), Output("deposition", "Deposition")
			}, {
				FloatParam("amount", "Amount", 0.35f, 0.0f, 1.0f),
				FloatParam("talusAngle", "Talus Angle", 32.0f, 15.0f, 60.0f),
				FloatParam("weathering", "Weathering", 0.45f, 0.0f, 1.0f)
			} },
			{ "treeAssetA", "Tree A", "Vegetation", { Input("density", "Density Mask", false, 1.0f) }, {}, {
				IntParam("seed", "Seed", 1337, 1.0f, 100000.0f),
				FloatParam("density", "Density", 0.45f, 0.0f, 1.0f),
				FloatParam("spacing", "Spacing", 55.0f, 10.0f, 250.0f),
				FloatParam("minScale", "Min Scale", 0.80f, 0.25f, 3.0f),
				FloatParam("maxScale", "Max Scale", 1.25f, 0.25f, 3.0f),
				FloatParam("maxSlope", "Max Slope", 15.0f, 0.0f, 75.0f),
				BoolParam("alignToSlope", "Align To Slope", false),
				FloatParam("maxTilt", "Max Tilt", 15.0f, 0.0f, 75.0f),
				FloatParam("renderDistance", "Render Distance", 1400.0f, 100.0f, 5000.0f)
			} },
			{ "treeAssetB", "Tree B", "Vegetation", { Input("density", "Density Mask", false, 1.0f) }, {}, {
				IntParam("seed", "Seed", 7331, 1.0f, 100000.0f),
				FloatParam("density", "Density", 0.30f, 0.0f, 1.0f),
				FloatParam("spacing", "Spacing", 65.0f, 10.0f, 250.0f),
				FloatParam("minScale", "Min Scale", 0.85f, 0.25f, 3.0f),
				FloatParam("maxScale", "Max Scale", 1.30f, 0.25f, 3.0f),
				FloatParam("maxSlope", "Max Slope", 15.0f, 0.0f, 75.0f),
				BoolParam("alignToSlope", "Align To Slope", false),
				FloatParam("maxTilt", "Max Tilt", 15.0f, 0.0f, 75.0f),
				FloatParam("renderDistance", "Render Distance", 1400.0f, 100.0f, 5000.0f)
			} },
			{ "grassAsset", "Grass", "Vegetation", { Input("density", "Density Mask", false, 1.0f) }, {}, {
				IntParam("seed", "Seed", 4242, 1.0f, 100000.0f),
				FloatParam("density", "Density", 0.95f, 0.0f, 1.0f),
				FloatParam("spacing", "Spacing", 3.0f, 1.0f, 30.0f),
				FloatParam("minScale", "Min Scale", 0.75f, 0.25f, 3.0f),
				FloatParam("maxScale", "Max Scale", 1.35f, 0.25f, 3.0f),
				FloatParam("maxSlope", "Max Slope", 38.0f, 0.0f, 75.0f),
				BoolParam("alignToSlope", "Align To Slope", false),
				FloatParam("maxTilt", "Max Tilt", 30.0f, 0.0f, 75.0f),
				FloatParam("renderDistance", "Render Distance", 180.0f, 50.0f, 600.0f)
			} },
			{ "vegetationAsset", "Vegetation Asset", "Vegetation", { Input("density", "Density Mask", false, 1.0f) }, {}, {
				StringParam("assetId", "Asset", "builtin.tree_a"),
				IntParam("seed", "Seed", 9157, 1.0f, 100000.0f),
				FloatParam("density", "Density", 0.45f, 0.0f, 1.0f),
				FloatParam("spacing", "Spacing", 55.0f, 1.0f, 250.0f),
				FloatParam("minScale", "Min Scale", 0.80f, 0.25f, 3.0f),
				FloatParam("maxScale", "Max Scale", 1.25f, 0.25f, 3.0f),
				FloatParam("maxSlope", "Max Slope", 15.0f, 0.0f, 75.0f),
				BoolParam("alignToSlope", "Align To Slope", false),
				FloatParam("maxTilt", "Max Tilt", 30.0f, 0.0f, 75.0f),
				FloatParam("renderDistance", "Render Distance", 1400.0f, 50.0f, 5000.0f)
			} },
			{ "outputHeight", "Output Height", "Outputs", { Input("height", "Height") }, {}, {} },
			{ "outputMask", "Output Mask", "Outputs", { Input("mask", "Mask") }, {}, { StringParam("target", "Target", "FinalMask") } }
		};
		return types;
	}

	const TerrainGraphNodeType* FindTerrainGraphNodeType(const std::string& typeId)
	{
		const auto& types = GetTerrainGraphNodeTypes();
		auto it = std::find_if(types.begin(), types.end(), [&](const TerrainGraphNodeType& type)
		{
			return type.typeId == typeId;
		});
		return it != types.end() ? &(*it) : nullptr;
	}

	TerrainGraphValueRange EstimateTerrainGraphOutputRange(const TerrainGraphAsset& asset, int nodeId, const std::string& portId, int depth)
	{
		if (depth > 32) return { 0.0f, 1.0f, false };
		const TerrainGraphNodeInstance* node = asset.FindNode(nodeId);
		if (!node) return { 0.0f, 1.0f, false };
		auto floatParam = [&](const char* id, float fallback)
		{
			const auto* parameter = FindParameter(*node, id);
			return parameter ? parameter->floatValue : fallback;
		};
		auto inputRange = [&](const char* inputId)
		{
			const TerrainGraphLink* link = asset.FindInputLink(nodeId, inputId);
			return link ? EstimateTerrainGraphOutputRange(asset, link->fromNodeId, link->fromPortId, depth + 1) : TerrainGraphValueRange{ 0.0f, 1.0f, false };
		};

		if (node->typeId == "constant")
		{
			float value = floatParam("value", 1.0f);
			return { value, value, true };
		}
		if (node->typeId == "noiseHeight" || node->typeId == "noiseMask" || node->typeId == "normalize" ||
			node->typeId == "smoothstep" || node->typeId == "thresholdMask" || node->typeId == "heightBandMask" ||
			node->typeId == "slopeMask" || node->typeId == "curvatureMask" || node->typeId == "posterizeMask")
		{
			return { 0.0f, 1.0f, true };
		}
		if (node->typeId == "particleErosion" || node->typeId == "thermalErosion")
		{
			return portId == "height" ? inputRange("height") : TerrainGraphValueRange{ 0.0f, 1.0f, true };
		}
		if (node->typeId == "drainageNetwork")
		{
			return { 0.0f, 1.0f, true };
		}
		if (node->typeId == "fitRange")
		{
			float a = floatParam("outMin", 0.0f);
			float b = floatParam("outMax", 1.0f);
			return { std::min(a, b), std::max(a, b), true };
		}
		if (node->typeId == "clamp" || node->typeId == "terrace")
		{
			float a = floatParam("min", 0.0f);
			float b = floatParam("max", 1.0f);
			return { std::min(a, b), std::max(a, b), true };
		}
		if (node->typeId == "blurMask" || node->typeId == "erodeMask" || node->typeId == "dilateMask")
		{
			return inputRange("mask");
		}
		if (node->typeId == "power")
		{
			auto value = inputRange("value");
			float exponent = std::max(floatParam("power", 1.0f), 0.0001f);
			return { std::pow(std::max(value.minimum, 0.0f), exponent), std::pow(std::max(value.maximum, 0.0f), exponent), value.valid };
		}
		if (node->typeId == "abs")
		{
			auto value = inputRange("value");
			return { 0.0f, std::max(std::abs(value.minimum), std::abs(value.maximum)), value.valid };
		}
		if (node->typeId == "invert")
		{
			auto value = inputRange("value");
			float pivot = floatParam("pivot", 1.0f);
			return { pivot - value.maximum, pivot - value.minimum, value.valid };
		}
		if (node->typeId == "lerp")
		{
			auto a = inputRange("a");
			auto b = inputRange("b");
			return { std::min(a.minimum, b.minimum), std::max(a.maximum, b.maximum), a.valid && b.valid };
		}
		if (node->typeId == "add" || node->typeId == "subtract" || node->typeId == "multiply" || node->typeId == "divide" ||
			node->typeId == "min" || node->typeId == "max")
		{
			auto a = inputRange("a");
			auto b = inputRange("b");
			if (!a.valid || !b.valid) return { 0.0f, 1.0f, false };
			if (node->typeId == "add") return { a.minimum + b.minimum, a.maximum + b.maximum, true };
			if (node->typeId == "subtract") return { a.minimum - b.maximum, a.maximum - b.minimum, true };
			if (node->typeId == "min") return { std::min(a.minimum, b.minimum), std::min(a.maximum, b.maximum), true };
			if (node->typeId == "max") return { std::max(a.minimum, b.minimum), std::max(a.maximum, b.maximum), true };
			if (node->typeId == "divide" && b.minimum <= 0.0f && b.maximum >= 0.0f) return { 0.0f, 1.0f, false };
			float values[4] = {
				node->typeId == "divide" ? a.minimum / b.minimum : a.minimum * b.minimum,
				node->typeId == "divide" ? a.minimum / b.maximum : a.minimum * b.maximum,
				node->typeId == "divide" ? a.maximum / b.minimum : a.maximum * b.minimum,
				node->typeId == "divide" ? a.maximum / b.maximum : a.maximum * b.maximum
			};
			return { *std::min_element(values, values + 4), *std::max_element(values, values + 4), true };
		}
		return { 0.0f, 1.0f, false };
	}

	const TerrainGraphPortDefinition* FindTerrainGraphInput(const TerrainGraphNodeType& type, const std::string& portId)
	{
		auto it = std::find_if(type.inputs.begin(), type.inputs.end(), [&](const TerrainGraphPortDefinition& port)
		{
			return port.id == portId;
		});
		return it != type.inputs.end() ? &(*it) : nullptr;
	}

	const TerrainGraphPortDefinition* FindTerrainGraphOutput(const TerrainGraphNodeType& type, const std::string& portId)
	{
		auto it = std::find_if(type.outputs.begin(), type.outputs.end(), [&](const TerrainGraphPortDefinition& port)
		{
			return port.id == portId;
		});
		return it != type.outputs.end() ? &(*it) : nullptr;
	}

	TerrainGraphValidationResult ValidateTerrainGraphAsset(const TerrainGraphAsset& asset)
	{
		TerrainGraphValidationResult result;
		std::unordered_map<int, int> indegree;
		std::unordered_map<int, std::vector<int>> outgoing;
		std::unordered_set<int> nodeIds;
		int particleErosionNodes = 0;
		int thermalErosionNodes = 0;
		int drainageNetworkNodes = 0;
		int particleErosionId = 0;
		int thermalErosionId = 0;
		std::vector<int> outputHeightIds;

		for (const auto& node : asset.nodes)
		{
			nodeIds.insert(node.id);
			indegree[node.id] = 0;
			const auto* type = FindTerrainGraphNodeType(node.typeId);
			if (!type)
			{
				result.valid = false;
				result.messages.push_back(node.displayName + ": unknown node type '" + node.typeId + "'");
				continue;
			}
			if (node.typeId == "particleErosion")
			{
				++particleErosionNodes;
				if (node.enabled) particleErosionId = node.id;
			}
			if (node.typeId == "thermalErosion")
			{
				++thermalErosionNodes;
				if (node.enabled) thermalErosionId = node.id;
			}
			if (node.typeId == "outputHeight" && node.enabled) outputHeightIds.push_back(node.id);
			if (node.typeId == "drainageNetwork") ++drainageNetworkNodes;
			for (const auto& input : type->inputs)
			{
				if (input.required && !asset.FindInputLink(node.id, input.id))
				{
					result.valid = false;
					result.messages.push_back(node.displayName + ": missing required input '" + input.displayName + "'");
				}
			}
		}
		if (particleErosionNodes > 1)
		{
			result.valid = false;
			result.messages.push_back("Only one Particle Erosion node is supported");
		}
		if (thermalErosionNodes > 1)
		{
			result.valid = false;
			result.messages.push_back("Only one Thermal Erosion node is supported");
		}
		if (drainageNetworkNodes > 1)
		{
			result.valid = false;
			result.messages.push_back("Only one Drainage Network node is supported");
		}

		for (const auto& link : asset.links)
		{
			const auto* fromNode = asset.FindNode(link.fromNodeId);
			const auto* toNode = asset.FindNode(link.toNodeId);
			if (!fromNode || !toNode)
			{
				result.valid = false;
				result.messages.push_back("Link references a missing node");
				continue;
			}
			const auto* fromType = FindTerrainGraphNodeType(fromNode->typeId);
			const auto* toType = FindTerrainGraphNodeType(toNode->typeId);
			if (!fromType || !toType)
			{
				continue;
			}
			const auto* fromPort = FindTerrainGraphOutput(*fromType, link.fromPortId);
			const auto* toPort = FindTerrainGraphInput(*toType, link.toPortId);
			if (!fromPort || !toPort)
			{
				result.valid = false;
				result.messages.push_back("Link has an unknown port");
				continue;
			}
			if (fromPort->valueType != toPort->valueType)
			{
				result.valid = false;
				result.messages.push_back(fromNode->displayName + " -> " + toNode->displayName + ": port type mismatch");
			}
			outgoing[fromNode->id].push_back(toNode->id);
			++indegree[toNode->id];
		}

		auto hasDirectHeightLink = [&](int start, int target)
		{
			if (start == 0 || target == 0) return false;
			return std::any_of(asset.links.begin(), asset.links.end(), [&](const TerrainGraphLink& link)
			{
				return link.fromNodeId == start && link.fromPortId == "height" &&
					link.toNodeId == target && link.toPortId == "height";
			});
		};
		if (thermalErosionId != 0)
		{
			bool reachesOutput = false;
			for (int outputId : outputHeightIds) reachesOutput = reachesOutput || hasDirectHeightLink(thermalErosionId, outputId);
			if (!reachesOutput)
			{
				result.valid = false;
				result.messages.push_back("Thermal Erosion must feed Output Height");
			}
			if (particleErosionId != 0 && !hasDirectHeightLink(particleErosionId, thermalErosionId))
			{
				result.valid = false;
				result.messages.push_back("Thermal Erosion must be downstream of Particle Erosion");
			}
		}

		std::queue<int> ready;
		for (const auto& [nodeId, degree] : indegree)
		{
			if (degree == 0)
			{
				ready.push(nodeId);
			}
		}

		while (!ready.empty())
		{
			int nodeId = ready.front();
			ready.pop();
			result.executionOrder.push_back(nodeId);
			for (int target : outgoing[nodeId])
			{
				if (--indegree[target] == 0)
				{
					ready.push(target);
				}
			}
		}

		if (result.executionOrder.size() != asset.nodes.size())
		{
			result.valid = false;
			result.messages.push_back("Graph contains a cycle");
		}
		if (result.messages.empty())
		{
			result.messages.push_back("Graph is valid");
		}
		return result;
	}

	TerrainGraphAsset BuildDefaultUniversalTerrainGraph()
	{
		TerrainGraphAsset asset;
		asset.useUniversalGraph = true;
		const auto* noise = FindTerrainGraphNodeType("noiseHeight");
		const auto* output = FindTerrainGraphNodeType("outputHeight");
		if (noise && output)
		{
			auto& noiseNode = asset.AddNode(*noise, ImVec2(0.0f, 0.0f));
			auto& outputNode = asset.AddNode(*output, ImVec2(360.0f, 30.0f));
			asset.AddLink(noiseNode.id, "height", outputNode.id, "height");
		}
		if (const auto* hydro = FindTerrainGraphNodeType("drainageNetwork"))
		{
			asset.AddNode(*hydro, ImVec2(0.0f, 190.0f));
		}
		if (noise && output)
		{
			const auto* particle = FindTerrainGraphNodeType("particleErosion");
			if (particle && asset.nodes.size() >= 2)
			{
				int noiseId = asset.nodes[0].id;
				int outputId = asset.nodes[1].id;
				asset.links.clear();
				auto& particleNode = asset.AddNode(*particle, ImVec2(360.0f, 20.0f));
				asset.AddLink(noiseId, "height", particleNode.id, "height");
				asset.AddLink(particleNode.id, "height", outputId, "height");
				asset.nodes[1].position = ImVec2(720.0f, 30.0f);
			}
		}
		if (noise && output)
		{
			const auto* thermal = FindTerrainGraphNodeType("thermalErosion");
			const auto* particle = FindTerrainGraphNodeType("particleErosion");
			if (thermal && particle)
			{
				auto particleIt = std::find_if(asset.nodes.begin(), asset.nodes.end(), [](const TerrainGraphNodeInstance& node)
				{
					return node.typeId == "particleErosion";
				});
				auto outputIt = std::find_if(asset.nodes.begin(), asset.nodes.end(), [](const TerrainGraphNodeInstance& node)
				{
					return node.typeId == "outputHeight";
				});
				if (particleIt != asset.nodes.end() && outputIt != asset.nodes.end())
				{
					const int particleId = particleIt->id;
					const int outputId = outputIt->id;
					asset.links.erase(std::remove_if(asset.links.begin(), asset.links.end(), [&](const TerrainGraphLink& link)
					{
						return link.fromNodeId == particleId && link.toNodeId == outputId;
					}), asset.links.end());
					auto& thermalNode = asset.AddNode(*thermal, ImVec2(720.0f, 20.0f));
					asset.AddLink(particleId, "height", thermalNode.id, "height");
					asset.AddLink(thermalNode.id, "height", outputId, "height");
					if (auto* movedOutput = asset.FindNode(outputId)) movedOutput->position = ImVec2(1080.0f, 30.0f);
				}
			}
		}
		const auto* vegetationNoise = FindTerrainGraphNodeType("noiseMask");
		const auto* treeA = FindTerrainGraphNodeType("treeAssetA");
		const auto* treeB = FindTerrainGraphNodeType("treeAssetB");
		const auto* grass = FindTerrainGraphNodeType("grassAsset");
		if (vegetationNoise && treeA && treeB && grass)
		{
			auto setIntParameter = [](TerrainGraphNodeInstance& node, const std::string& id, int value)
			{
				auto found = std::find_if(node.parameters.begin(), node.parameters.end(), [&](const TerrainGraphParameter& parameter)
				{
					return parameter.id == id;
				});
				if (found != node.parameters.end()) found->intValue = value;
			};
			auto setFloatParameter = [](TerrainGraphNodeInstance& node, const std::string& id, float value)
			{
				auto found = std::find_if(node.parameters.begin(), node.parameters.end(), [&](const TerrainGraphParameter& parameter)
				{
					return parameter.id == id;
				});
				if (found != node.parameters.end()) found->floatValue = value;
			};
			int forestNoiseId = asset.AddNode(*vegetationNoise, ImVec2(0.0f, 430.0f)).id;
			int treeAId = asset.AddNode(*treeA, ImVec2(360.0f, 390.0f)).id;
			int treeBId = asset.AddNode(*treeB, ImVec2(360.0f, 590.0f)).id;
			int grassNoiseId = asset.AddNode(*vegetationNoise, ImVec2(0.0f, 760.0f)).id;
			int grassId = asset.AddNode(*grass, ImVec2(360.0f, 790.0f)).id;
			if (auto* node = asset.FindNode(forestNoiseId))
			{
				node->displayName = "Forest Distribution";
				setIntParameter(*node, "seed", 901);
				setFloatParameter(*node, "scale", 420.0f);
				setFloatParameter(*node, "frequency", 0.8f);
			}
			if (auto* node = asset.FindNode(grassNoiseId))
			{
				node->displayName = "Grass Distribution";
				setIntParameter(*node, "seed", 1901);
				setFloatParameter(*node, "scale", 150.0f);
				setFloatParameter(*node, "frequency", 1.25f);
			}
			asset.AddLink(forestNoiseId, "mask", treeAId, "density");
			asset.AddLink(forestNoiseId, "mask", treeBId, "density");
			asset.AddLink(grassNoiseId, "mask", grassId, "density");
		}
		return asset;
	}

	const char* TerrainGraphValueTypeName(TerrainGraphValueType type)
	{
		switch (type)
		{
		case TerrainGraphValueType::FloatMask: return "FloatMask";
		case TerrainGraphValueType::VectorMask: return "VectorMask";
		default: return "Unknown";
		}
	}
}
