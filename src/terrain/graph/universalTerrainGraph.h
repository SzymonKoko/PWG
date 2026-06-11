#ifndef SRC_TERRAIN_GRAPH_UNIVERSAL_TERRAIN_GRAPH_H
#define SRC_TERRAIN_GRAPH_UNIVERSAL_TERRAIN_GRAPH_H

#include "imgui.h"

#include <string>
#include <vector>

namespace pwg
{
	enum class TerrainGraphValueType
	{
		FloatMask,
		VectorMask
	};

	enum class TerrainGraphParameterType
	{
		Float,
		Int,
		Bool,
		String
	};

	struct TerrainGraphParameter
	{
		std::string id;
		std::string displayName;
		TerrainGraphParameterType type{ TerrainGraphParameterType::Float };
		float floatValue{ 0.0f };
		float minValue{ 0.0f };
		float maxValue{ 1.0f };
		int intValue{ 0 };
		bool boolValue{ false };
		std::string stringValue;
	};

	struct TerrainGraphPortDefinition
	{
		std::string id;
		std::string displayName;
		TerrainGraphValueType valueType{ TerrainGraphValueType::FloatMask };
		bool required{ true };
		float defaultValue{ 0.0f };
	};

	struct TerrainGraphNodeType
	{
		std::string typeId;
		std::string displayName;
		std::string category;
		std::vector<TerrainGraphPortDefinition> inputs;
		std::vector<TerrainGraphPortDefinition> outputs;
		std::vector<TerrainGraphParameter> parameters;
	};

	struct TerrainGraphNodeInstance
	{
		int id{ 0 };
		std::string typeId;
		std::string displayName;
		bool enabled{ true };
		ImVec2 position{ 0.0f, 0.0f };
		std::vector<TerrainGraphParameter> parameters;
	};

	struct TerrainGraphLink
	{
		int id{ 0 };
		int fromNodeId{ 0 };
		std::string fromPortId;
		int toNodeId{ 0 };
		std::string toPortId;
	};

	struct TerrainGraphValidationResult
	{
		bool valid{ true };
		std::vector<std::string> messages;
		std::vector<int> executionOrder;
	};

	struct TerrainGraphAsset
	{
		std::vector<TerrainGraphNodeInstance> nodes;
		std::vector<TerrainGraphLink> links;
		ImVec2 pan{ 48.0f, 48.0f };
		float zoom{ 1.0f };
		int nextNodeId{ 1 };
		int nextLinkId{ 1 };
		bool useUniversalGraph{ false };

		TerrainGraphNodeInstance* FindNode(int nodeId);
		const TerrainGraphNodeInstance* FindNode(int nodeId) const;
		const TerrainGraphLink* FindInputLink(int nodeId, const std::string& portId) const;
		TerrainGraphNodeInstance& AddNode(const TerrainGraphNodeType& type, ImVec2 position);
		bool RemoveNode(int nodeId);
		bool AddLink(int fromNodeId, const std::string& fromPortId, int toNodeId, const std::string& toPortId);
		bool RemoveLink(int linkId);
		void Clear();
	};

	const std::vector<TerrainGraphNodeType>& GetTerrainGraphNodeTypes();
	const TerrainGraphNodeType* FindTerrainGraphNodeType(const std::string& typeId);
	const TerrainGraphPortDefinition* FindTerrainGraphInput(const TerrainGraphNodeType& type, const std::string& portId);
	const TerrainGraphPortDefinition* FindTerrainGraphOutput(const TerrainGraphNodeType& type, const std::string& portId);
	TerrainGraphValidationResult ValidateTerrainGraphAsset(const TerrainGraphAsset& asset);
	TerrainGraphAsset BuildDefaultUniversalTerrainGraph();
	const char* TerrainGraphValueTypeName(TerrainGraphValueType type);
}

#endif // SRC_TERRAIN_GRAPH_UNIVERSAL_TERRAIN_GRAPH_H
