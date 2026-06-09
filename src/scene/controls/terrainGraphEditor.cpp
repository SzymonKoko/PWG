#include "terrainGraphEditor.h"

#include "terrain/chunk/chunkManager.h"
#include "terrain/graph/passes/terrainPass.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <unordered_map>
#include <utility>

namespace pwg::controls
{
	namespace
	{
		const char* PassTypeName(TerrainPassTypes type)
		{
			switch (type)
			{
			case TerrainPassTypes::GENERATION: return "Generation";
			case TerrainPassTypes::MASK: return "Mask";
			case TerrainPassTypes::COMPOSITION: return "Composition";
			case TerrainPassTypes::ANALYSIS: return "Analysis";
			case TerrainPassTypes::SIMULATION: return "Simulation";
			case TerrainPassTypes::HYDROLOGY: return "Hydrology";
			case TerrainPassTypes::EROSION: return "Erosion";
			default: return "Unknown";
			}
		}

		ImU32 PassTypeColor(TerrainPassTypes type)
		{
			switch (type)
			{
			case TerrainPassTypes::GENERATION: return IM_COL32(70, 142, 205, 255);
			case TerrainPassTypes::MASK: return IM_COL32(76, 168, 122, 255);
			case TerrainPassTypes::HYDROLOGY: return IM_COL32(70, 122, 205, 255);
			case TerrainPassTypes::EROSION: return IM_COL32(198, 132, 70, 255);
			case TerrainPassTypes::COMPOSITION: return IM_COL32(170, 112, 190, 255);
			case TerrainPassTypes::ANALYSIS: return IM_COL32(180, 170, 85, 255);
			case TerrainPassTypes::SIMULATION: return IM_COL32(120, 160, 185, 255);
			default: return IM_COL32(120, 120, 120, 255);
			}
		}

		const char* ParameterTypeName(ParameterType type)
		{
			switch (type)
			{
			case ParameterType::Float: return "float";
			case ParameterType::Int: return "int";
			case ParameterType::Bool: return "bool";
			case ParameterType::Vec2: return "vec2";
			case ParameterType::Vec3: return "vec3";
			case ParameterType::Mat4: return "mat4";
			case ParameterType::List: return "list";
			default: return "unknown";
			}
		}

		void DrawConnection(ImDrawList* drawList, ImVec2 start, ImVec2 end, ImU32 color, float thickness)
		{
			float dx = end.x - start.x;
			float dy = end.y - start.y;
			float direction = dx >= 0.0f ? 1.0f : -1.0f;
			float tangent = std::clamp(std::abs(dx) * 0.38f + std::abs(dy) * 0.12f, 24.0f, 320.0f);
			ImVec2 cp0(start.x + tangent * direction, start.y);
			ImVec2 cp1(end.x - tangent * direction, end.y);
			drawList->AddBezierCubic(start, cp0, cp1, end, color, thickness, 24);
		}

		ImVec2 Add(ImVec2 a, ImVec2 b)
		{
			return ImVec2(a.x + b.x, a.y + b.y);
		}

		ImVec2 Sub(ImVec2 a, ImVec2 b)
		{
			return ImVec2(a.x - b.x, a.y - b.y);
		}

		ImVec2 Mul(ImVec2 value, float scale)
		{
			return ImVec2(value.x * scale, value.y * scale);
		}

		float DistanceSquared(ImVec2 a, ImVec2 b)
		{
			float dx = a.x - b.x;
			float dy = a.y - b.y;
			return dx * dx + dy * dy;
		}

		ImVec2 Lerp(ImVec2 a, ImVec2 b, float t)
		{
			return ImVec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
		}

		float DistancePointToSegmentSquared(ImVec2 point, ImVec2 a, ImVec2 b)
		{
			ImVec2 ab = Sub(b, a);
			float lengthSquared = ab.x * ab.x + ab.y * ab.y;
			if (lengthSquared <= 0.0001f)
			{
				return DistanceSquared(point, a);
			}
			float t = ((point.x - a.x) * ab.x + (point.y - a.y) * ab.y) / lengthSquared;
			t = std::clamp(t, 0.0f, 1.0f);
			return DistanceSquared(point, ImVec2(a.x + ab.x * t, a.y + ab.y * t));
		}

		float DistanceToConnectionSquared(ImVec2 point, ImVec2 start, ImVec2 end)
		{
			float dx = end.x - start.x;
			float dy = end.y - start.y;
			float direction = dx >= 0.0f ? 1.0f : -1.0f;
			float tangent = std::clamp(std::abs(dx) * 0.38f + std::abs(dy) * 0.12f, 24.0f, 320.0f);
			ImVec2 cp0(start.x + tangent * direction, start.y);
			ImVec2 cp1(end.x - tangent * direction, end.y);

			float bestDistance = std::numeric_limits<float>::max();
			ImVec2 previous = start;
			for (int i = 1; i <= 24; ++i)
			{
				float t = static_cast<float>(i) / 24.0f;
				ImVec2 a = Lerp(start, cp0, t);
				ImVec2 b = Lerp(cp0, cp1, t);
				ImVec2 c = Lerp(cp1, end, t);
				ImVec2 d = Lerp(a, b, t);
				ImVec2 e = Lerp(b, c, t);
				ImVec2 current = Lerp(d, e, t);
				bestDistance = std::min(bestDistance, DistancePointToSegmentSquared(point, previous, current));
				previous = current;
			}
			return bestDistance;
		}

		int FindPortIndex(const std::vector<TerrainGraphPort>& ports, const std::string& name)
		{
			for (int i = 0; i < static_cast<int>(ports.size()); ++i)
			{
				if (ports[i].name == name)
				{
					return i;
				}
			}
			return -1;
		}

		int FindNodeIndexByPassName(const TerrainGraphModel& model, const std::string& passName)
		{
			for (int i = 0; i < static_cast<int>(model.nodes.size()); ++i)
			{
				if (model.nodes[i].passName == passName)
				{
					return i;
				}
			}
			return -1;
		}

		bool HasProposedEdge(const TerrainGraphViewState& viewState, const std::string& fromPass, const std::string& toPass, const std::string& maskName)
		{
			return std::any_of(viewState.proposedEdges.begin(), viewState.proposedEdges.end(), [&](const TerrainGraphProposedEdge& edge)
			{
				return edge.fromPass == fromPass && edge.toPass == toPass && edge.maskName == maskName;
			});
		}

		bool HasDependencyEdge(const TerrainGraphModel& model, int fromNode, int toNode, const std::string& maskName)
		{
			return std::any_of(model.dependencyEdges.begin(), model.dependencyEdges.end(), [&](const TerrainGraphEdge& edge)
			{
				return edge.fromNode == fromNode && edge.toNode == toNode && edge.maskName == maskName;
			});
		}

		const char* ProposedEdgeStatusName(const TerrainGraphModel& model, const TerrainGraphViewState& viewState, int edgeIndex)
		{
			if (edgeIndex < 0 || edgeIndex >= static_cast<int>(viewState.proposedEdges.size()))
			{
				return "invalid";
			}

			const auto& edge = viewState.proposedEdges[edgeIndex];
			int fromNode = FindNodeIndexByPassName(model, edge.fromPass);
			int toNode = FindNodeIndexByPassName(model, edge.toPass);
			if (fromNode < 0 || toNode < 0 || fromNode == toNode)
			{
				return "invalid";
			}
			if (FindPortIndex(model.nodes[fromNode].outputs, edge.maskName) < 0 || FindPortIndex(model.nodes[toNode].inputs, edge.maskName) < 0)
			{
				return "invalid";
			}
			if (HasDependencyEdge(model, fromNode, toNode, edge.maskName))
			{
				return "existing";
			}
			for (int i = 0; i < edgeIndex; ++i)
			{
				const auto& previous = viewState.proposedEdges[i];
				if (previous.fromPass == edge.fromPass && previous.toPass == edge.toPass && previous.maskName == edge.maskName)
				{
					return "duplicate";
				}
			}
			return "valid";
		}

		bool CanAddProposedEdge(const TerrainGraphModel& model, const TerrainGraphViewState& viewState, int fromNode, int toNode, const std::string& maskName)
		{
			if (fromNode < 0 || toNode < 0 || fromNode == toNode)
			{
				return false;
			}
			if (FindPortIndex(model.nodes[fromNode].outputs, maskName) < 0 || FindPortIndex(model.nodes[toNode].inputs, maskName) < 0)
			{
				return false;
			}
			if (HasDependencyEdge(model, fromNode, toNode, maskName))
			{
				return false;
			}
			return !HasProposedEdge(viewState, model.nodes[fromNode].passName, model.nodes[toNode].passName, maskName);
		}

		ImU32 ProposedEdgeStatusColor(const char* status)
		{
			if (std::strcmp(status, "valid") == 0)
			{
				return IM_COL32(112, 220, 150, 255);
			}
			if (std::strcmp(status, "existing") == 0)
			{
				return IM_COL32(90, 165, 220, 255);
			}
			if (std::strcmp(status, "duplicate") == 0)
			{
				return IM_COL32(230, 190, 90, 255);
			}
			return IM_COL32(230, 105, 105, 255);
		}

		void DrawPortList(const char* label, const std::vector<std::string>& ports, ImU32 color)
		{
			ImGui::TextDisabled("%s", label);
			if (ports.empty())
			{
				ImGui::TextDisabled("  none");
				return;
			}

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			for (const auto& port : ports)
			{
				ImVec2 cursor = ImGui::GetCursorScreenPos();
				drawList->AddCircleFilled(ImVec2(cursor.x + 5.0f, cursor.y + ImGui::GetTextLineHeight() * 0.5f), 4.0f, color);
				ImGui::Indent(16.0f);
				ImGui::Text("%s", port.c_str());
				ImGui::Unindent(16.0f);
			}
		}

		bool DrawParameterControl(UIParameters& param)
		{
			static const char* noiseTypes[] = { "OpenSimplex2", "OpenSimplex2S", "Cellular", "Perlin", "Value Cubic", "Value" };
			static const char* fractalTypes[] = { "None", "FBm", "Ridged", "Ping Pong" };

			switch (param.type)
			{
			case ParameterType::Float:
				return ImGui::SliderFloat(param.name.c_str(), static_cast<float*>(param.data), param.min, param.max);
			case ParameterType::Int:
				return ImGui::SliderInt(param.name.c_str(), static_cast<int*>(param.data), static_cast<int>(param.min), static_cast<int>(param.max));
			case ParameterType::Bool:
				return ImGui::Checkbox(param.name.c_str(), static_cast<bool*>(param.data));
			case ParameterType::List:
				if (param.name == "Noise Type")
				{
					return ImGui::Combo(param.name.c_str(), static_cast<int*>(param.data), noiseTypes, IM_ARRAYSIZE(noiseTypes));
				}
				if (param.name == "Fractal Type")
				{
					return ImGui::Combo(param.name.c_str(), static_cast<int*>(param.data), fractalTypes, IM_ARRAYSIZE(fractalTypes));
				}
				return ImGui::SliderInt(param.name.c_str(), static_cast<int*>(param.data), static_cast<int>(param.min), static_cast<int>(param.max));
			default:
				ImGui::TextDisabled("%s (%s) is not editable yet.", param.name.c_str(), ParameterTypeName(param.type));
				return false;
			}
		}

		TerrainGraphModel BuildGraphModel(const std::vector<std::shared_ptr<TerrainPass>>& passes)
		{
			TerrainGraphModel model;
			model.nodes.reserve(passes.size());

			std::unordered_map<std::string, int> outputProducer;
			for (int i = 0; i < static_cast<int>(passes.size()); ++i)
			{
				const auto& pass = passes[i];
				TerrainGraphNode node;
				node.passIndex = i;
				node.passName = pass->GetName();
				node.typeName = PassTypeName(pass->GetPassType());

				for (const auto& input : pass->GetInputMasks())
				{
					node.inputs.push_back({ input, TerrainGraphPortDirection::Input });
				}
				for (const auto& output : pass->GetOutputMasks())
				{
					node.outputs.push_back({ output, TerrainGraphPortDirection::Output });
					if (!outputProducer.contains(output))
					{
						outputProducer[output] = i;
					}
				}

				model.nodes.push_back(node);
			}

			for (int consumer = 0; consumer < static_cast<int>(passes.size()); ++consumer)
			{
				for (const auto& input : passes[consumer]->GetInputMasks())
				{
					auto producerIt = outputProducer.find(input);
					if (producerIt == outputProducer.end() || producerIt->second == consumer)
					{
						continue;
					}

					TerrainGraphEdge edge;
					edge.fromNode = producerIt->second;
					edge.toNode = consumer;
					edge.maskName = input;

					bool exists = std::any_of(model.dependencyEdges.begin(), model.dependencyEdges.end(), [&](const TerrainGraphEdge& existing)
					{
						return existing.fromNode == edge.fromNode && existing.toNode == edge.toNode && existing.maskName == edge.maskName;
					});
					if (!exists)
					{
						model.dependencyEdges.push_back(edge);
					}
				}
			}

			for (int i = 0; i + 1 < static_cast<int>(passes.size()); ++i)
			{
				TerrainGraphEdge edge;
				edge.fromNode = i;
				edge.toNode = i + 1;
				edge.maskName = "execution";
				model.executionEdges.push_back(edge);
			}

			return model;
		}

		ImVec2 DefaultNodePosition(int index)
		{
			const ImVec2 nodeSize(230.0f, 210.0f);
			const float horizontalSpacing = 128.0f;
			const float rowSpacing = 92.0f;
			const int nodesPerRow = 4;
			int row = index / nodesPerRow;
			int column = index % nodesPerRow;
			bool reverseRow = (row % 2) == 1;
			int visualColumn = reverseRow ? (nodesPerRow - 1 - column) : column;
			return ImVec2(
				static_cast<float>(visualColumn) * (nodeSize.x + horizontalSpacing),
				static_cast<float>(row) * (nodeSize.y + rowSpacing));
		}

		bool EnsureNodeLayout(TerrainGraphViewState& viewState, const std::vector<std::shared_ptr<TerrainPass>>& passes, bool resetLayout)
		{
			bool layoutChanged = resetLayout || viewState.nodes.size() != passes.size();
			for (int i = 0; !layoutChanged && i < static_cast<int>(passes.size()); ++i)
			{
				layoutChanged = viewState.nodes[i].passName != passes[i]->GetName();
			}
			if (!layoutChanged)
			{
				return false;
			}

			std::unordered_map<std::string, ImVec2> previousPositions;
			if (!resetLayout)
			{
				for (const auto& node : viewState.nodes)
				{
					previousPositions[node.passName] = node.position;
				}
			}

			viewState.nodes.clear();
			viewState.nodes.reserve(passes.size());
			for (int i = 0; i < static_cast<int>(passes.size()); ++i)
			{
				const std::string name = passes[i]->GetName();
				auto previousIt = previousPositions.find(name);
				TerrainGraphNodeLayout node;
				node.passName = name;
				node.position = previousIt != previousPositions.end() ? previousIt->second : DefaultNodePosition(i);
				viewState.nodes.push_back(node);
			}
			return true;
		}

		bool FitGraphToView(TerrainGraphViewState& viewState, ImVec2 viewportSize)
		{
			if (viewState.nodes.empty())
			{
				return false;
			}

			const ImVec2 nodeSize(230.0f, 210.0f);
			ImVec2 minPos = viewState.nodes.front().position;
			ImVec2 maxPos(viewState.nodes.front().position.x + nodeSize.x, viewState.nodes.front().position.y + nodeSize.y);
			for (const auto& node : viewState.nodes)
			{
				minPos.x = std::min(minPos.x, node.position.x);
				minPos.y = std::min(minPos.y, node.position.y);
				maxPos.x = std::max(maxPos.x, node.position.x + nodeSize.x);
				maxPos.y = std::max(maxPos.y, node.position.y + nodeSize.y);
			}

			const float padding = 48.0f;
			float contentWidth = std::max(1.0f, maxPos.x - minPos.x);
			float contentHeight = std::max(1.0f, maxPos.y - minPos.y);
			float zoomX = (viewportSize.x - padding * 2.0f) / contentWidth;
			float zoomY = (viewportSize.y - padding * 2.0f) / contentHeight;
			viewState.zoom = std::clamp(std::min(zoomX, zoomY), 0.30f, 2.75f);
			viewState.pan = ImVec2(
				padding - minPos.x * viewState.zoom,
				padding - minPos.y * viewState.zoom);
			return true;
		}

		bool DrawGraphCanvas(const TerrainGraphModel& model, std::vector<std::shared_ptr<TerrainPass>>& passes, int& selectedPassIndex, TerrainGraphViewState& viewState, bool resetLayout, bool resetView, bool fitGraph, bool* viewChanged)
		{
			const ImVec2 nodeSize(230.0f, 210.0f);
			if (viewChanged)
			{
				*viewChanged = false;
			}

			ImVec2 viewportSize = ImGui::GetContentRegionAvail();
			viewportSize.x = std::max(viewportSize.x, 320.0f);
			viewportSize.y = std::max(viewportSize.y, 260.0f);
			bool layoutChanged = EnsureNodeLayout(viewState, passes, resetLayout);
			if (resetView)
			{
				viewState.pan = ImVec2(40.0f, 42.0f);
				viewState.zoom = 1.0f;
				layoutChanged = true;
			}
			if (fitGraph)
			{
				layoutChanged = FitGraphToView(viewState, viewportSize) || layoutChanged;
			}
			if (layoutChanged && viewChanged)
			{
				*viewChanged = true;
			}

			ImVec2 origin = ImGui::GetCursorScreenPos();
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImGui::Dummy(viewportSize);
			ImVec2 viewportMax(origin.x + viewportSize.x, origin.y + viewportSize.y);
			bool hovered = ImGui::IsItemHovered();
			ImGuiIO& io = ImGui::GetIO();
			bool capturedInput = hovered;
			if (viewState.connectionDragActive)
			{
				capturedInput = true;
			}

			if (hovered && io.MouseWheel != 0.0f)
			{
				float oldZoom = viewState.zoom;
				viewState.zoom = std::clamp(viewState.zoom * (io.MouseWheel > 0.0f ? 1.12f : 0.89f), 0.30f, 2.75f);
				ImVec2 mouseLocal(io.MousePos.x - origin.x, io.MousePos.y - origin.y);
				viewState.pan = Sub(mouseLocal, Mul(Sub(mouseLocal, viewState.pan), viewState.zoom / oldZoom));
				if (viewChanged)
				{
					*viewChanged = true;
				}
			}

			if (hovered && (ImGui::IsMouseDragging(ImGuiMouseButton_Right) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle)))
			{
				capturedInput = true;
				viewState.pan.x += io.MouseDelta.x;
				viewState.pan.y += io.MouseDelta.y;
				if (viewChanged && (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f))
				{
					*viewChanged = true;
				}
			}

			if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				viewState.pan = ImVec2(40.0f, 42.0f);
				viewState.zoom = 1.0f;
				if (viewChanged)
				{
					*viewChanged = true;
				}
			}

			drawList->PushClipRect(origin, viewportMax, true);
			drawList->AddRectFilled(origin, viewportMax, IM_COL32(14, 16, 18, 52));

			float gridStep = 64.0f * viewState.zoom;
			if (gridStep >= 12.0f)
			{
				float startX = origin.x + std::fmod(viewState.pan.x, gridStep);
				float startY = origin.y + std::fmod(viewState.pan.y, gridStep);
				for (float x = startX; x < viewportMax.x; x += gridStep)
				{
					drawList->AddLine(ImVec2(x, origin.y), ImVec2(x, viewportMax.y), IM_COL32(255, 255, 255, 18));
				}
				for (float y = startY; y < viewportMax.y; y += gridStep)
				{
					drawList->AddLine(ImVec2(origin.x, y), ImVec2(viewportMax.x, y), IM_COL32(255, 255, 255, 18));
				}
			}

			auto nodeWorldPosition = [&](int index)
			{
				return viewState.nodes[index].position;
			};
			auto toScreen = [&](ImVec2 world)
			{
				return Add(origin, Add(viewState.pan, Mul(world, viewState.zoom)));
			};
			ImVec2 scaledNodeSize = Mul(nodeSize, viewState.zoom);
			auto portPosition = [&](int nodeIndex, TerrainGraphPortDirection direction, int portIndex)
			{
				ImVec2 nodePos = toScreen(nodeWorldPosition(nodeIndex));
				float y = nodePos.y + (64.0f + static_cast<float>(portIndex) * 17.0f) * viewState.zoom;
				if (direction == TerrainGraphPortDirection::Input)
				{
					return ImVec2(nodePos.x, y);
				}
				return ImVec2(nodePos.x + scaledNodeSize.x, y);
			};

			for (const auto& edge : model.dependencyEdges)
			{
				int outputIndex = FindPortIndex(model.nodes[edge.fromNode].outputs, edge.maskName);
				int inputIndex = FindPortIndex(model.nodes[edge.toNode].inputs, edge.maskName);
				if (outputIndex < 0 || inputIndex < 0)
				{
					continue;
				}
				ImVec2 fromPort = portPosition(edge.fromNode, TerrainGraphPortDirection::Output, outputIndex);
				ImVec2 toPort = portPosition(edge.toNode, TerrainGraphPortDirection::Input, inputIndex);
				DrawConnection(drawList, fromPort, toPort, IM_COL32(72, 150, 170, 165), std::max(1.0f, 2.0f * viewState.zoom));
			}

			int hoveredProposedEdge = -1;
			for (int edgeIndex = 0; edgeIndex < static_cast<int>(viewState.proposedEdges.size()); ++edgeIndex)
			{
				const auto& proposedEdge = viewState.proposedEdges[edgeIndex];
				int fromNode = FindNodeIndexByPassName(model, proposedEdge.fromPass);
				int toNode = FindNodeIndexByPassName(model, proposedEdge.toPass);
				if (fromNode < 0 || toNode < 0)
				{
					continue;
				}
				int outputIndex = FindPortIndex(model.nodes[fromNode].outputs, proposedEdge.maskName);
				int inputIndex = FindPortIndex(model.nodes[toNode].inputs, proposedEdge.maskName);
				if (outputIndex < 0 || inputIndex < 0)
				{
					continue;
				}
				ImVec2 fromPort = portPosition(fromNode, TerrainGraphPortDirection::Output, outputIndex);
				ImVec2 toPort = portPosition(toNode, TerrainGraphPortDirection::Input, inputIndex);
				float hitRadius = std::max(7.0f, 6.0f * viewState.zoom);
				bool edgeHovered = hovered && DistanceToConnectionSquared(io.MousePos, fromPort, toPort) <= hitRadius * hitRadius;
				if (edgeHovered)
				{
					hoveredProposedEdge = edgeIndex;
				}
				ImU32 color = edgeHovered ? IM_COL32(235, 170, 255, 255) : IM_COL32(185, 130, 230, 205);
				float thickness = edgeHovered ? std::max(2.0f, 3.4f * viewState.zoom) : std::max(1.2f, 2.4f * viewState.zoom);
				DrawConnection(drawList, fromPort, toPort, color, thickness);
			}
			if (hoveredProposedEdge >= 0)
			{
				capturedInput = true;
				ImGui::SetTooltip("Press C to cut proposed link");
				if (!io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_C))
				{
					viewState.proposedEdges.erase(viewState.proposedEdges.begin() + hoveredProposedEdge);
					if (viewChanged)
					{
						*viewChanged = true;
					}
				}
			}

			for (const auto& edge : model.executionEdges)
			{
				ImVec2 from = toScreen(nodeWorldPosition(edge.fromNode));
				ImVec2 to = toScreen(nodeWorldPosition(edge.toNode));
				ImVec2 fromCenter(from.x + scaledNodeSize.x * 0.5f, from.y + scaledNodeSize.y * 0.5f);
				ImVec2 toCenter(to.x + scaledNodeSize.x * 0.5f, to.y + scaledNodeSize.y * 0.5f);
				DrawConnection(drawList, fromCenter, toCenter, IM_COL32(98, 102, 108, 150), std::max(1.0f, 1.6f * viewState.zoom));
			}

			for (int i = 0; i < static_cast<int>(passes.size()); ++i)
			{
				auto& pass = passes[i];
				const auto& node = model.nodes[i];
				ImVec2 pos = toScreen(nodeWorldPosition(i));
				ImVec2 max(pos.x + scaledNodeSize.x, pos.y + scaledNodeSize.y);
				bool selected = i == selectedPassIndex;
				ImU32 accent = PassTypeColor(pass->GetPassType());
				ImU32 fill = selected ? IM_COL32(37, 45, 52, 255) : IM_COL32(29, 31, 34, 255);
				ImU32 border = selected ? accent : IM_COL32(65, 69, 74, 255);

				bool onScreen = max.x >= origin.x && pos.x <= viewportMax.x && max.y >= origin.y && pos.y <= viewportMax.y;
				bool outputPortPressed = false;
				if (onScreen && hovered)
				{
					ImGui::PushID(i);
					for (int outputIndex = 0; outputIndex < static_cast<int>(node.outputs.size()); ++outputIndex)
					{
						ImVec2 portPos = portPosition(i, TerrainGraphPortDirection::Output, outputIndex);
						float hitRadius = std::max(10.0f, 9.0f * viewState.zoom);
						ImGui::PushID(outputIndex);
						ImGui::SetCursorScreenPos(ImVec2(portPos.x - hitRadius, portPos.y - hitRadius));
						ImGui::InvisibleButton("output_port", ImVec2(hitRadius * 2.0f, hitRadius * 2.0f));
						bool portActive = ImGui::IsItemActive();
						if (ImGui::IsItemHovered())
						{
							ImGui::SetTooltip("Drag output: %s", node.outputs[outputIndex].name.c_str());
						}
						if (!viewState.connectionDragActive && ImGui::IsItemClicked(ImGuiMouseButton_Left))
						{
							outputPortPressed = true;
							viewState.connectionDragActive = true;
							viewState.connectionDragNode = i;
							viewState.connectionDragPort = node.outputs[outputIndex].name;
							selectedPassIndex = i;
							capturedInput = true;
						}
						if (portActive)
						{
							outputPortPressed = true;
							capturedInput = true;
						}
						ImGui::PopID();
					}
					ImGui::PopID();
				}

				if (onScreen && !outputPortPressed)
				{
					ImGui::SetCursorScreenPos(pos);
					ImGui::PushID(i);
					ImGui::InvisibleButton("node", scaledNodeSize);
					if (ImGui::IsItemClicked())
					{
						selectedPassIndex = i;
					}
					if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
					{
						capturedInput = true;
						viewState.nodes[i].position.x += io.MouseDelta.x / viewState.zoom;
						viewState.nodes[i].position.y += io.MouseDelta.y / viewState.zoom;
						if (viewChanged && (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f))
						{
							*viewChanged = true;
						}
					}
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("%s", pass->GetName().c_str());
					}
					ImGui::PopID();
				}

				float rounding = 4.0f * viewState.zoom;
				drawList->AddRectFilled(pos, max, fill, rounding);
				drawList->AddRect(pos, max, border, rounding, 0, selected ? std::max(1.0f, 2.0f * viewState.zoom) : 1.0f);
				drawList->AddRectFilled(pos, ImVec2(max.x, pos.y + 5.0f * viewState.zoom), accent, rounding);
				std::string indexLabel = "#" + std::to_string(i + 1);
				float fontSize = ImGui::GetFontSize() * std::clamp(viewState.zoom, 0.55f, 1.35f);
				drawList->AddText(ImGui::GetFont(), fontSize, ImVec2(pos.x + 10.0f * viewState.zoom, pos.y + 16.0f * viewState.zoom), IM_COL32(150, 155, 160, 255), indexLabel.c_str());
				drawList->AddText(ImGui::GetFont(), fontSize, ImVec2(pos.x + 10.0f * viewState.zoom, pos.y + 36.0f * viewState.zoom), IM_COL32(230, 232, 235, 255), node.passName.c_str());
				drawList->AddText(ImGui::GetFont(), fontSize, ImVec2(pos.x + 10.0f * viewState.zoom, pos.y + 58.0f * viewState.zoom), IM_COL32(170, 175, 180, 255),
					node.typeName.c_str());

				float portRadius = std::max(3.0f, 4.0f * viewState.zoom);
				float portFontSize = ImGui::GetFontSize() * std::clamp(viewState.zoom, 0.45f, 0.95f);
				for (int inputIndex = 0; inputIndex < static_cast<int>(node.inputs.size()); ++inputIndex)
				{
					ImVec2 portPos = portPosition(i, TerrainGraphPortDirection::Input, inputIndex);
					drawList->AddCircleFilled(portPos, portRadius, IM_COL32(92, 150, 220, 255));
					bool portHovered = hovered && DistanceSquared(io.MousePos, portPos) <= std::max(12.0f, 10.0f * viewState.zoom) * std::max(12.0f, 10.0f * viewState.zoom);
					bool compatibleTarget = viewState.connectionDragActive && i != viewState.connectionDragNode && node.inputs[inputIndex].name == viewState.connectionDragPort;
					if (portHovered || compatibleTarget)
					{
						ImU32 ringColor = compatibleTarget ? IM_COL32(112, 220, 150, 230) : IM_COL32(210, 220, 235, 190);
						drawList->AddCircle(portPos, portRadius + 4.0f * viewState.zoom, ringColor, 16, std::max(1.0f, 1.6f * viewState.zoom));
					}
					if (viewState.zoom >= 0.48f)
					{
						drawList->AddText(ImGui::GetFont(), portFontSize, ImVec2(portPos.x + 9.0f * viewState.zoom, portPos.y - 7.0f * viewState.zoom), IM_COL32(165, 190, 220, 220), node.inputs[inputIndex].name.c_str());
					}
				}
				for (int outputIndex = 0; outputIndex < static_cast<int>(node.outputs.size()); ++outputIndex)
				{
					ImVec2 portPos = portPosition(i, TerrainGraphPortDirection::Output, outputIndex);
					drawList->AddCircleFilled(portPos, portRadius, IM_COL32(92, 190, 130, 255));
					bool portHovered = hovered && DistanceSquared(io.MousePos, portPos) <= std::max(12.0f, 10.0f * viewState.zoom) * std::max(12.0f, 10.0f * viewState.zoom);
					bool activeSource = viewState.connectionDragActive && i == viewState.connectionDragNode && node.outputs[outputIndex].name == viewState.connectionDragPort;
					if (portHovered || activeSource)
					{
						drawList->AddCircle(portPos, portRadius + 4.0f * viewState.zoom, IM_COL32(210, 240, 210, 220), 16, std::max(1.0f, 1.6f * viewState.zoom));
					}
					if (viewState.zoom >= 0.48f)
					{
						ImVec2 textSize = ImGui::CalcTextSize(node.outputs[outputIndex].name.c_str());
						drawList->AddText(ImGui::GetFont(), portFontSize, ImVec2(portPos.x - (textSize.x * (portFontSize / ImGui::GetFontSize())) - 10.0f * viewState.zoom, portPos.y - 7.0f * viewState.zoom), IM_COL32(170, 220, 185, 220), node.outputs[outputIndex].name.c_str());
					}
				}
			}

			if (viewState.connectionDragActive)
			{
				int sourceNode = viewState.connectionDragNode;
				int outputIndex = sourceNode >= 0 && sourceNode < static_cast<int>(model.nodes.size())
					? FindPortIndex(model.nodes[sourceNode].outputs, viewState.connectionDragPort)
					: -1;
				if (sourceNode >= 0 && outputIndex >= 0)
				{
					ImVec2 fromPort = portPosition(sourceNode, TerrainGraphPortDirection::Output, outputIndex);
					int targetNode = -1;
					int targetInput = -1;
					for (int nodeIndex = 0; nodeIndex < static_cast<int>(model.nodes.size()); ++nodeIndex)
					{
						if (nodeIndex == sourceNode)
						{
							continue;
						}
						int inputIndex = FindPortIndex(model.nodes[nodeIndex].inputs, viewState.connectionDragPort);
						if (inputIndex < 0)
						{
							continue;
						}
						ImVec2 inputPos = portPosition(nodeIndex, TerrainGraphPortDirection::Input, inputIndex);
						float hitRadius = std::max(12.0f, 10.0f * viewState.zoom);
						if (DistanceSquared(io.MousePos, inputPos) <= hitRadius * hitRadius)
						{
							targetNode = nodeIndex;
							targetInput = inputIndex;
							break;
						}
					}

					ImU32 previewColor = targetNode >= 0 ? IM_COL32(112, 220, 150, 230) : IM_COL32(230, 190, 90, 210);
					DrawConnection(drawList, fromPort, io.MousePos, previewColor, std::max(1.5f, 2.6f * viewState.zoom));
					if (targetNode >= 0)
					{
						ImVec2 inputPos = portPosition(targetNode, TerrainGraphPortDirection::Input, targetInput);
						drawList->AddCircle(inputPos, std::max(8.0f, 8.0f * viewState.zoom), IM_COL32(112, 220, 150, 255), 16, 2.0f);
					}

					if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
					{
						if (targetNode >= 0)
						{
							selectedPassIndex = targetNode;
							const auto& fromPass = model.nodes[sourceNode].passName;
							const auto& toPass = model.nodes[targetNode].passName;
							const auto& maskName = viewState.connectionDragPort;
							if (CanAddProposedEdge(model, viewState, sourceNode, targetNode, maskName))
							{
								viewState.proposedEdges.push_back({ fromPass, toPass, maskName });
								if (viewChanged)
								{
									*viewChanged = true;
								}
							}
						}
						viewState.connectionDragActive = false;
						viewState.connectionDragNode = -1;
						viewState.connectionDragPort.clear();
					}
				}
				else
				{
					viewState.connectionDragActive = false;
					viewState.connectionDragNode = -1;
					viewState.connectionDragPort.clear();
				}
			}

			std::string zoomLabel = "Zoom " + std::to_string(static_cast<int>(viewState.zoom * 100.0f)) + "%";
			drawList->AddText(ImVec2(origin.x + 10.0f, viewportMax.y - 24.0f), IM_COL32(190, 195, 200, 210), zoomLabel.c_str());
			drawList->AddText(ImVec2(origin.x + 100.0f, viewportMax.y - 24.0f), IM_COL32(140, 145, 150, 190), "Left drag node/port | C cuts hovered purple link | Wheel zoom | Right/Middle drag pan");
			drawList->PopClipRect();
			return capturedInput;
		}

		bool DrawInspector(std::shared_ptr<ChunkManager> chunkManager, const TerrainGraphModel& model, std::vector<std::shared_ptr<TerrainPass>>& passes, int& selectedPassIndex, TerrainGraphViewState& viewState, bool* viewChanged)
		{
			auto& pass = passes[selectedPassIndex];
			auto params = pass->GetParameters();
			bool changed = false;

			if (ImGui::BeginCombo("Selected pass", pass->GetName().c_str()))
			{
				for (int i = 0; i < static_cast<int>(passes.size()); ++i)
				{
					std::string label = "#" + std::to_string(i + 1) + " " + passes[i]->GetName();
					bool selected = i == selectedPassIndex;
					if (ImGui::Selectable(label.c_str(), selected))
					{
						selectedPassIndex = i;
					}
					if (selected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			ImGui::Spacing();

			ImGui::Text("%s", pass->GetName().c_str());
			ImGui::TextDisabled("%s pass", PassTypeName(pass->GetPassType()));
			ImGui::Separator();
			ImGui::Text("Execution index: %d", selectedPassIndex + 1);
			ImGui::Text("Parameter count: %d", static_cast<int>(params.size()));
			ImGui::Spacing();
			if (ImGui::CollapsingHeader("Ports", ImGuiTreeNodeFlags_DefaultOpen))
			{
				DrawPortList("Inputs", pass->GetInputMasks(), IM_COL32(92, 150, 220, 255));
				ImGui::Spacing();
				DrawPortList("Outputs", pass->GetOutputMasks(), IM_COL32(92, 190, 130, 255));
			}
			ImGui::Spacing();
			if (ImGui::CollapsingHeader("Proposed links", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::TextDisabled("Visual staging only. Terrain execution is unchanged.");
				if (viewState.proposedEdges.empty())
				{
					ImGui::TextDisabled("No proposed links.");
				}
				else
				{
					if (ImGui::Button("Clear proposals"))
					{
						viewState.proposedEdges.clear();
						if (viewChanged)
						{
							*viewChanged = true;
						}
					}
					ImGui::Separator();

					for (int edgeIndex = 0; edgeIndex < static_cast<int>(viewState.proposedEdges.size());)
					{
						const auto& edge = viewState.proposedEdges[edgeIndex];
						const char* status = ProposedEdgeStatusName(model, viewState, edgeIndex);
						ImGui::PushID(edgeIndex);
						ImGui::TextWrapped("%s -> %s", edge.fromPass.c_str(), edge.toPass.c_str());
						ImGui::TextDisabled("%s", edge.maskName.c_str());
						ImGui::SameLine();
						ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(ProposedEdgeStatusColor(status)), "%s", status);
						if (ImGui::SmallButton("Remove"))
						{
							viewState.proposedEdges.erase(viewState.proposedEdges.begin() + edgeIndex);
							if (viewChanged)
							{
								*viewChanged = true;
							}
							ImGui::PopID();
							continue;
						}
						ImGui::Separator();
						ImGui::PopID();
						++edgeIndex;
					}
				}
			}
			ImGui::Spacing();
			if (ImGui::Button("Regenerate Terrain"))
			{
				chunkManager->SetNeedRegenerate(true);
				changed = true;
			}
			if (pass->GetPassType() == TerrainPassTypes::HYDROLOGY || pass->GetPassType() == TerrainPassTypes::EROSION)
			{
				ImGui::SameLine();
				if (ImGui::Button("Step Simulation"))
				{
					chunkManager->StepSimulation();
					changed = true;
				}
			}
			ImGui::Spacing();
			ImGui::TextDisabled("Parameters");
			ImGui::Separator();

			if (params.empty())
			{
				ImGui::TextDisabled("No exposed parameters.");
				return changed;
			}

			ImGui::PushItemWidth(-1.0f);
			for (auto& param : params)
			{
				ImGui::PushID(param.name.c_str());
				if (DrawParameterControl(param))
				{
					changed = true;
				}
				if (param.type == ParameterType::Float || param.type == ParameterType::Int)
				{
					ImGui::TextDisabled("Range: %.3f - %.3f", param.min, param.max);
				}
				ImGui::TextDisabled("%s", ParameterTypeName(param.type));
				ImGui::Spacing();
				ImGui::PopID();
			}
			ImGui::PopItemWidth();

			if (changed && pass->GetPassType() != TerrainPassTypes::HYDROLOGY && pass->GetPassType() != TerrainPassTypes::EROSION)
			{
				chunkManager->SetNeedRegenerate(true);
			}

			return changed;
		}
	}

	bool TerrainGraphEditor::Show(std::shared_ptr<ChunkManager> chunkManager, TerrainGraphViewState& viewState, bool* inputCaptured, bool* viewChanged)
	{
		if (inputCaptured)
		{
			*inputCaptured = false;
		}
		if (viewChanged)
		{
			*viewChanged = false;
		}

		if (!chunkManager)
		{
			ImGui::TextDisabled("Terrain chunk manager is not available.");
			return false;
		}

		auto terrainComputePipeline = chunkManager->GetTerrainComputePipeline();
		if (!terrainComputePipeline || !terrainComputePipeline->GetGraph())
		{
			ImGui::TextDisabled("Terrain graph is not available.");
			return false;
		}

		auto& passes = terrainComputePipeline->GetGraph()->GetPasses();
		static int selectedPassIndex = 0;
		if (passes.empty())
		{
			selectedPassIndex = 0;
			ImGui::TextDisabled("Terrain graph has no passes.");
			return false;
		}
		TerrainGraphModel model = BuildGraphModel(passes);
		selectedPassIndex = std::clamp(selectedPassIndex, 0, static_cast<int>(passes.size()) - 1);
		bool changed = false;

		ImGui::Text("TerrainGraph");
		ImGui::SameLine();
		ImGui::TextDisabled("%d passes | execution order", static_cast<int>(passes.size()));
		ImGui::Separator();

		ImVec2 available = ImGui::GetContentRegionAvail();
		float detailsWidth = std::clamp(available.x * 0.34f, 240.0f, 360.0f);
		float graphWidth = std::max(available.x - detailsWidth - 8.0f, 320.0f);
		bool fitGraph = false;
		bool resetLayout = false;
		bool resetView = false;

		if (ImGui::Button("Fit graph"))
		{
			fitGraph = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset layout"))
		{
			resetLayout = true;
			fitGraph = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset view"))
		{
			resetView = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("Links are visual proposals until applied in a later stage.");

		ImGui::BeginChild("TerrainGraphCanvas", ImVec2(graphWidth, 0.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
		bool canvasViewChanged = false;
		bool canvasCapturedInput = DrawGraphCanvas(model, passes, selectedPassIndex, viewState, resetLayout, resetView, fitGraph, &canvasViewChanged);
		if (inputCaptured)
		{
			*inputCaptured = canvasCapturedInput;
		}
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("TerrainGraphInspector", ImVec2(0.0f, 0.0f), true);
		bool inspectorViewChanged = false;
		changed = DrawInspector(chunkManager, model, passes, selectedPassIndex, viewState, &inspectorViewChanged);
		ImGui::EndChild();
		if (viewChanged)
		{
			*viewChanged = canvasViewChanged || inspectorViewChanged;
		}

		return changed;
	}
}
