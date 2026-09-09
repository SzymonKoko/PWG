#ifndef SRC_SCENE_CONTROLS_TERRAIN_GRAPH_EDITOR_H
#define SRC_SCENE_CONTROLS_TERRAIN_GRAPH_EDITOR_H

#include "imgui.h"

#include <memory>
#include <string>
#include <vector>

namespace pwg
{
	class ChunkManager;
	class VegetationAssetRegistry;
}

namespace pwg::controls
{
	struct TerrainGraphNodeLayout
	{
		std::string passName;
		ImVec2 position{ 0.0f, 0.0f };
	};

	struct TerrainGraphProposedEdge
	{
		std::string fromPass;
		std::string toPass;
		std::string maskName;
	};

	struct TerrainGraphViewState
	{
		ImVec2 pan{ 40.0f, 42.0f };
		float zoom{ 1.0f };
		std::vector<TerrainGraphNodeLayout> nodes;
		std::vector<TerrainGraphProposedEdge> proposedEdges;
		bool connectionDragActive{ false };
		int connectionDragNode{ -1 };
		std::string connectionDragPort;
	};

	enum class TerrainGraphPortDirection
	{
		Input,
		Output
	};

	struct TerrainGraphPort
	{
		std::string name;
		TerrainGraphPortDirection direction{ TerrainGraphPortDirection::Input };
	};

	struct TerrainGraphNode
	{
		int passIndex{ -1 };
		std::string passName;
		std::string typeName;
		std::vector<TerrainGraphPort> inputs;
		std::vector<TerrainGraphPort> outputs;
	};

	struct TerrainGraphEdge
	{
		int fromNode{ -1 };
		int toNode{ -1 };
		std::string maskName;
	};

	struct TerrainGraphModel
	{
		std::vector<TerrainGraphNode> nodes;
		std::vector<TerrainGraphEdge> dependencyEdges;
		std::vector<TerrainGraphEdge> executionEdges;
	};

	class TerrainGraphEditor
	{
	public:
		static void ResetHistory();
		static bool Show(std::shared_ptr<ChunkManager> chunkManager, TerrainGraphViewState& viewState, std::shared_ptr<VegetationAssetRegistry> assetRegistry = nullptr, bool* inputCaptured = nullptr, bool* viewChanged = nullptr);
	};
}

#endif // !SRC_SCENE_CONTROLS_TERRAIN_GRAPH_EDITOR_H
