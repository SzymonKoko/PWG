#include "universalTerrainGraph.h"

#include <algorithm>
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
			{ "terrace", "Terrace", "Terrain", { Input("height", "Height") }, { Output("height", "Height") }, {
				FloatParam("min", "Min Height", 0.0f, -20000.0f, 20000.0f),
				FloatParam("max", "Max Height", 1.0f, -20000.0f, 20000.0f),
				IntParam("steps", "Steps", 8, 2.0f, 64.0f)
			} },
			{ "posterizeMask", "Posterize Mask", "Terrain", { Input("mask", "Mask") }, { Output("mask", "Mask") }, {
				IntParam("steps", "Steps", 4, 2.0f, 64.0f)
			} },
			{ "hydraulicSimulation", "Hydraulic Simulation", "Simulation", {}, {}, {
				BoolParam("autoWaterSimulation", "Auto Run", false),
				IntParam("stepsPerUpdate", "Steps Per Update", 1, 1.0f, 16.0f),
				IntParam("waterStepsPerSecond", "Steps Per Second", 8, 1.0f, 60.0f),
				IntParam("waterSubsteps", "Substeps", 2, 1.0f, 8.0f),
				FloatParam("rainRate", "Rain Rate", 0.004f, 0.0f, 0.08f),
				FloatParam("rainElevationBias", "Rain Elevation Bias", 0.35f, 0.0f, 1.0f),
				FloatParam("rainMinHeight", "Rain Min Height", 0.0f, -20000.0f, 20000.0f),
				FloatParam("rainMaxHeight", "Rain Max Height", 1.2f, -20000.0f, 20000.0f),
				FloatParam("evaporation", "Evaporation", 0.00004f, 0.0f, 0.01f),
				FloatParam("infiltrationRate", "Infiltration", 0.00003f, 0.0f, 0.01f),
				FloatParam("boundaryDrain", "Boundary Drain", 0.92f, 0.0f, 1.0f),
				FloatParam("maxWaterDepth", "Max Water Depth", 2.5f, 0.05f, 20.0f),
				FloatParam("pipeFlow", "Pipe Flow", 0.38f, 0.0f, 2.0f),
				FloatParam("pipeDamping", "Pipe Damping", 0.28f, 0.0f, 1.0f),
				FloatParam("flowConcentration", "Flow Concentration", 0.40f, 0.0f, 2.0f),
				BoolParam("useDiagonalFlow", "Diagonal Flow", true)
			} },
			{ "streamErosion", "Stream Erosion", "Simulation", {}, {}, {
				BoolParam("erosionEnabled", "Enabled", true),
				FloatParam("sedimentCapacity", "Sediment Capacity", 4.0f, 0.0f, 20.0f),
				FloatParam("erosionStrength", "Channel Carving", 0.20f, 0.0f, 0.6f),
				FloatParam("depositionStrength", "Deposition", 0.014f, 0.0f, 0.2f),
				FloatParam("flatteningStrength", "Flattening", 0.25f, 0.0f, 1.0f),
				FloatParam("minWaterToErode", "Min Water To Erode", 0.0008f, 0.0f, 0.05f),
				FloatParam("minFlowToErode", "Min Flow To Erode", 0.0003f, 0.0f, 0.05f),
				FloatParam("maxHeightChange", "Max Height Change", 0.018f, 0.0f, 0.2f),
				FloatParam("maxErosionDepth", "Max Erosion Depth", 1.0f, 0.0f, 20.0f),
				FloatParam("maxDepositHeight", "Max Deposit Height", 0.06f, 0.0f, 2.0f),
				FloatParam("flowAccumulationGrowth", "Flow Accum Growth", 0.90f, 0.0f, 2.0f),
				FloatParam("flowAccumulationDecay", "Flow Accum Decay", 0.008f, 0.0f, 0.1f),
				FloatParam("riverChannelGrowth", "River Growth", 0.22f, 0.0f, 1.0f),
				FloatParam("riverChannelDecay", "River Decay", 0.004f, 0.0f, 0.1f),
				FloatParam("riverChannelInfluence", "River Influence", 0.55f, 0.0f, 2.0f),
				FloatParam("riverChannelDepth", "River Depth", 0.14f, 0.0f, 2.0f)
			} },
			{ "particleErosion", "Particle Erosion", "Simulation", {}, {}, {
				BoolParam("particleErosionEnabled", "Enabled", false),
				IntParam("bakeParticleSteps", "Bake Steps", 24, 1.0f, 512.0f),
				IntParam("bakeStepsPerFrame", "Bake Steps Per Frame", 1, 1.0f, 32.0f),
				IntParam("bakeUploadInterval", "Upload Interval", 4, 1.0f, 64.0f),
				IntParam("particlesPerStep", "Particles Per Step", 2500, 100.0f, 120000.0f),
				IntParam("dropletLifetime", "Droplet Lifetime", 36, 1.0f, 256.0f),
				IntParam("particleSeed", "Seed", 1337, 1.0f, 100000.0f),
				FloatParam("particleInertia", "Inertia", 0.22f, 0.0f, 0.95f),
				FloatParam("particleGravity", "Gravity", 5.5f, 0.1f, 20.0f),
				FloatParam("particleInitialWater", "Initial Water", 1.0f, 0.0f, 10.0f),
				FloatParam("particleEvaporation", "Evaporation", 0.018f, 0.0f, 0.2f),
				FloatParam("particleMinSlope", "Min Slope", 0.0008f, 0.0f, 0.05f),
				FloatParam("particleCapacity", "Capacity", 5.0f, 0.1f, 20.0f),
				FloatParam("particleErosionStrength", "Erosion Strength", 0.045f, 0.0f, 0.3f),
				FloatParam("particleDepositionStrength", "Deposition Strength", 0.18f, 0.0f, 0.5f),
				FloatParam("particleBrushRadius", "Brush Radius", 2.0f, 0.5f, 5.0f),
				FloatParam("particleDepositRadius", "Deposit Radius", 5.0f, 0.5f, 12.0f),
				FloatParam("particleChannelStrength", "Channel Strength", 0.04f, 0.0f, 0.2f),
				FloatParam("particleFlowStrength", "Flow Strength", 0.015f, 0.0f, 0.1f)
			} },
			{ "lakeCleanup", "Lake Cleanup", "Simulation", {}, {}, {
				FloatParam("lakeMinDepth", "Lake Min Depth", 0.012f, 0.005f, 0.15f),
				FloatParam("lakeLeveling", "Surface Leveling", 0.48f, 0.0f, 0.8f),
				FloatParam("lakeFillEnabled", "Particle Lake Fill", 1.0f, 0.0f, 1.0f),
				FloatParam("lakeMinFlow", "Lake Min Flow", 0.015f, 0.0f, 1.0f),
				FloatParam("lakeMaxDepth", "Lake Max Depth", 2.5f, 0.05f, 10.0f),
				FloatParam("lakeBedSmoothing", "Bed Smoothing", 0.06f, 0.0f, 0.5f),
				FloatParam("lakeWaterSmoothing", "Water Smoothing", 0.30f, 0.0f, 1.0f),
				FloatParam("lakeInfiltrationScale", "Infiltration Scale", 0.03f, 0.0f, 1.0f)
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
			for (const auto& input : type->inputs)
			{
				if (input.required && !asset.FindInputLink(node.id, input.id))
				{
					result.valid = false;
					result.messages.push_back(node.displayName + ": missing required input '" + input.displayName + "'");
				}
			}
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
		const auto* noise = FindTerrainGraphNodeType("noiseHeight");
		const auto* output = FindTerrainGraphNodeType("outputHeight");
		if (noise && output)
		{
			auto& noiseNode = asset.AddNode(*noise, ImVec2(0.0f, 0.0f));
			auto& outputNode = asset.AddNode(*output, ImVec2(360.0f, 30.0f));
			asset.AddLink(noiseNode.id, "height", outputNode.id, "height");
		}
		if (const auto* hydro = FindTerrainGraphNodeType("hydraulicSimulation"))
		{
			asset.AddNode(*hydro, ImVec2(0.0f, 190.0f));
		}
		if (const auto* stream = FindTerrainGraphNodeType("streamErosion"))
		{
			asset.AddNode(*stream, ImVec2(360.0f, 190.0f));
		}
		if (const auto* particle = FindTerrainGraphNodeType("particleErosion"))
		{
			asset.AddNode(*particle, ImVec2(720.0f, 190.0f));
		}
		if (const auto* lake = FindTerrainGraphNodeType("lakeCleanup"))
		{
			asset.AddNode(*lake, ImVec2(1080.0f, 190.0f));
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
