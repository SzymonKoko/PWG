#include "terrainGraphEditor.h"

#include "terrain/chunk/chunkManager.h"
#include "terrain/graph/passes/terrainPass.h"
#include "terrain/graph/universalTerrainGraph.h"
#include "vegetation/vegetationAssetRegistry.h"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <limits>
#include <optional>
#include <unordered_map>
#include <unordered_set>
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

		constexpr size_t UniversalGraphHistoryLimit = 100;

		struct UniversalGraphEditorState
		{
			std::vector<int> selectedNodeIds;
			std::vector<TerrainGraphNodeInstance> clipboardNodes;
			std::vector<TerrainGraphLink> clipboardLinks;
			std::vector<TerrainGraphAsset> undoStack;
			std::vector<TerrainGraphAsset> redoStack;
			TerrainGraphAsset observedAsset;
			TerrainGraphAsset transactionStart;
			bool hasObservedAsset{ false };
			bool transactionActive{ false };
			bool showMaskPreviews{ true };
		};

		UniversalGraphEditorState& UniversalEditorState()
		{
			static UniversalGraphEditorState state;
			return state;
		}

		bool GraphParametersEqual(const TerrainGraphParameter& lhs, const TerrainGraphParameter& rhs)
		{
			return lhs.id == rhs.id && lhs.displayName == rhs.displayName && lhs.type == rhs.type &&
				lhs.floatValue == rhs.floatValue && lhs.minValue == rhs.minValue && lhs.maxValue == rhs.maxValue &&
				lhs.intValue == rhs.intValue && lhs.boolValue == rhs.boolValue && lhs.stringValue == rhs.stringValue;
		}

		bool GraphNodesEqual(const TerrainGraphNodeInstance& lhs, const TerrainGraphNodeInstance& rhs)
		{
			if (lhs.id != rhs.id || lhs.typeId != rhs.typeId || lhs.displayName != rhs.displayName || lhs.enabled != rhs.enabled ||
				lhs.position.x != rhs.position.x || lhs.position.y != rhs.position.y || lhs.parameters.size() != rhs.parameters.size())
			{
				return false;
			}
			for (size_t i = 0; i < lhs.parameters.size(); ++i)
			{
				if (!GraphParametersEqual(lhs.parameters[i], rhs.parameters[i])) return false;
			}
			return true;
		}

		bool GraphAssetsEqual(const TerrainGraphAsset& lhs, const TerrainGraphAsset& rhs)
		{
			if (lhs.nodes.size() != rhs.nodes.size() || lhs.links.size() != rhs.links.size() ||
				lhs.nextNodeId != rhs.nextNodeId || lhs.nextLinkId != rhs.nextLinkId || lhs.useUniversalGraph != rhs.useUniversalGraph)
			{
				return false;
			}
			for (size_t i = 0; i < lhs.nodes.size(); ++i)
			{
				if (!GraphNodesEqual(lhs.nodes[i], rhs.nodes[i])) return false;
			}
			for (size_t i = 0; i < lhs.links.size(); ++i)
			{
				const auto& a = lhs.links[i];
				const auto& b = rhs.links[i];
				if (a.id != b.id || a.fromNodeId != b.fromNodeId || a.fromPortId != b.fromPortId ||
					a.toNodeId != b.toNodeId || a.toPortId != b.toPortId) return false;
			}
			return true;
		}

		void PushGraphSnapshot(std::vector<TerrainGraphAsset>& stack, const TerrainGraphAsset& asset)
		{
			if (!stack.empty() && GraphAssetsEqual(stack.back(), asset)) return;
			if (stack.size() >= UniversalGraphHistoryLimit) stack.erase(stack.begin());
			stack.push_back(asset);
		}

		void RestoreGraphAsset(TerrainGraphAsset& target, const TerrainGraphAsset& snapshot)
		{
			const ImVec2 pan = target.pan;
			const float zoom = target.zoom;
			target = snapshot;
			target.pan = pan;
			target.zoom = zoom;
		}

		void EnsureGraphHistory(const TerrainGraphAsset& asset)
		{
			auto& state = UniversalEditorState();
			if (!state.hasObservedAsset)
			{
				state.observedAsset = asset;
				state.hasObservedAsset = true;
			}
		}

		void RecordGraphHistory(const TerrainGraphAsset& asset, bool interactionActive)
		{
			auto& state = UniversalEditorState();
			EnsureGraphHistory(asset);
			const bool changed = !GraphAssetsEqual(state.observedAsset, asset);
			if (changed)
			{
				if (interactionActive)
				{
					if (!state.transactionActive)
					{
						state.transactionStart = state.observedAsset;
						state.transactionActive = true;
					}
					state.observedAsset = asset;
					state.redoStack.clear();
					return;
				}
				PushGraphSnapshot(state.undoStack, state.transactionActive ? state.transactionStart : state.observedAsset);
				state.redoStack.clear();
				state.observedAsset = asset;
				state.transactionActive = false;
				return;
			}
			if (state.transactionActive && !interactionActive)
			{
				PushGraphSnapshot(state.undoStack, state.transactionStart);
				state.transactionActive = false;
			}
		}

		bool UndoGraph(TerrainGraphAsset& asset)
		{
			auto& state = UniversalEditorState();
			EnsureGraphHistory(asset);
			if (state.transactionActive)
			{
				PushGraphSnapshot(state.undoStack, state.transactionStart);
				state.transactionActive = false;
			}
			if (state.undoStack.empty()) return false;
			PushGraphSnapshot(state.redoStack, asset);
			RestoreGraphAsset(asset, state.undoStack.back());
			state.undoStack.pop_back();
			state.observedAsset = asset;
			return true;
		}

		bool RedoGraph(TerrainGraphAsset& asset)
		{
			auto& state = UniversalEditorState();
			EnsureGraphHistory(asset);
			if (state.redoStack.empty()) return false;
			PushGraphSnapshot(state.undoStack, asset);
			RestoreGraphAsset(asset, state.redoStack.back());
			state.redoStack.pop_back();
			state.observedAsset = asset;
			state.transactionActive = false;
			return true;
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
			static const char* curvatureModes[] = { "Both", "Ridges", "Valleys" };

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

			if (changed && pass->GetPassType() != TerrainPassTypes::HYDROLOGY)
			{
				chunkManager->SetNeedRegenerate(true);
			}

			return changed;
		}

		TerrainGraphParameter* FindNodeParameter(TerrainGraphNodeInstance& node, const std::string& id)
		{
			auto it = std::find_if(node.parameters.begin(), node.parameters.end(), [&](const TerrainGraphParameter& param)
			{
				return param.id == id;
			});
			return it != node.parameters.end() ? &(*it) : nullptr;
		}

		bool DrawGraphParameterControl(TerrainGraphParameter& param, const std::shared_ptr<VegetationAssetRegistry>& assetRegistry)
		{
			static const char* noiseTypes[] = { "OpenSimplex2", "OpenSimplex2S", "Cellular", "Perlin", "Value Cubic", "Value" };
			static const char* fractalTypes[] = { "None", "FBm", "Ridged", "Ping Pong" };
			static const char* curvatureModes[] = { "Both", "Ridges", "Valleys" };
			bool changed = false;
			ImGui::TextWrapped("%s", param.displayName.c_str());
			ImGui::PushID(param.id.c_str());
			ImGui::SetNextItemWidth(-1.0f);
			switch (param.type)
			{
			case TerrainGraphParameterType::Float:
				changed = ImGui::SliderFloat("##value", &param.floatValue, param.minValue, param.maxValue);
				break;
			case TerrainGraphParameterType::Int:
				if (param.id == "noiseType")
				{
					int selectedNoise = std::clamp(param.intValue, 0, static_cast<int>(IM_ARRAYSIZE(noiseTypes)) - 1);
					if (ImGui::Combo("##value", &selectedNoise, noiseTypes, IM_ARRAYSIZE(noiseTypes)))
					{
						param.intValue = selectedNoise;
						changed = true;
					}
				}
				else if (param.id == "fractalType")
				{
					int selectedFractal = std::clamp(param.intValue, 0, static_cast<int>(IM_ARRAYSIZE(fractalTypes)) - 1);
					if (ImGui::Combo("##value", &selectedFractal, fractalTypes, IM_ARRAYSIZE(fractalTypes)))
					{
						param.intValue = selectedFractal;
						changed = true;
					}
				}
				else if (param.id == "mode")
				{
					int selectedMode = std::clamp(param.intValue, 0, static_cast<int>(IM_ARRAYSIZE(curvatureModes)) - 1);
					if (ImGui::Combo("##value", &selectedMode, curvatureModes, IM_ARRAYSIZE(curvatureModes)))
					{
						param.intValue = selectedMode;
						changed = true;
					}
				}
				else
				{
					changed = ImGui::SliderInt("##value", &param.intValue, static_cast<int>(param.minValue), static_cast<int>(param.maxValue));
				}
				break;
			case TerrainGraphParameterType::Bool:
				changed = ImGui::Checkbox("##value", &param.boolValue);
				break;
			case TerrainGraphParameterType::String:
			{
				if (param.id == "assetId" && assetRegistry)
				{
					const VegetationAssetDefinition* selected = assetRegistry->Find(param.stringValue);
					std::string preview = selected ? selected->name : "Missing: " + param.stringValue;
					if (ImGui::BeginCombo("##value", preview.c_str()))
					{
						for (const auto& asset : assetRegistry->Assets())
						{
							bool isSelected = asset.id == param.stringValue;
							const char* kind = asset.kind == VegetationKind::Grass ? "Grass" : (asset.kind == VegetationKind::Rock ? "Rock" : "Tree");
							std::string label = asset.name + " [" + kind + "]";
							if (ImGui::Selectable(label.c_str(), isSelected))
							{
								param.stringValue = asset.id;
								changed = true;
							}
							if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", asset.id.c_str());
							if (isSelected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					break;
				}
				char buffer[128]{};
				std::snprintf(buffer, sizeof(buffer), "%s", param.stringValue.c_str());
				if (ImGui::InputText("##value", buffer, sizeof(buffer)))
				{
					param.stringValue = buffer;
					changed = true;
				}
				break;
			}
			default:
				break;
			}

			if ((param.type == TerrainGraphParameterType::Float || param.type == TerrainGraphParameterType::Int) && param.id != "noiseType" && param.id != "fractalType" && param.id != "mode")
			{
				ImGui::TextDisabled("Range");
				float inputWidth = std::max(70.0f, (ImGui::GetContentRegionAvail().x - 6.0f) * 0.5f);
				if (param.type == TerrainGraphParameterType::Float)
				{
					float minValue = param.minValue;
					float maxValue = param.maxValue;
					ImGui::TextDisabled("Min");
					ImGui::SameLine(inputWidth + 6.0f);
					ImGui::TextDisabled("Max");
					ImGui::SetNextItemWidth(inputWidth);
					bool minChanged = ImGui::InputFloat("##rangeMin", &minValue, 0.0f, 0.0f, "%.3f");
					ImGui::SameLine();
					ImGui::SetNextItemWidth(inputWidth);
					bool maxChanged = ImGui::InputFloat("##rangeMax", &maxValue, 0.0f, 0.0f, "%.3f");
					if (minChanged || maxChanged)
					{
						if (minValue > maxValue)
						{
							std::swap(minValue, maxValue);
						}
						if (std::abs(maxValue - minValue) < 0.0001f)
						{
							maxValue = minValue + 0.0001f;
						}
						param.minValue = minValue;
						param.maxValue = maxValue;
						param.floatValue = std::clamp(param.floatValue, param.minValue, param.maxValue);
						changed = true;
					}
				}
				else
				{
					int minValue = static_cast<int>(param.minValue);
					int maxValue = static_cast<int>(param.maxValue);
					ImGui::TextDisabled("Min");
					ImGui::SameLine(inputWidth + 6.0f);
					ImGui::TextDisabled("Max");
					ImGui::SetNextItemWidth(inputWidth);
					bool minChanged = ImGui::InputInt("##rangeMin", &minValue, 0);
					ImGui::SameLine();
					ImGui::SetNextItemWidth(inputWidth);
					bool maxChanged = ImGui::InputInt("##rangeMax", &maxValue, 0);
					if (minChanged || maxChanged)
					{
						if (minValue > maxValue)
						{
							std::swap(minValue, maxValue);
						}
						if (maxValue == minValue)
						{
							++maxValue;
						}
						param.minValue = static_cast<float>(minValue);
						param.maxValue = static_cast<float>(maxValue);
						param.intValue = std::clamp(param.intValue, minValue, maxValue);
						changed = true;
					}
				}
				ImGui::TextDisabled("Range: %.3f - %.3f", param.minValue, param.maxValue);
			}
			ImGui::PopID();
			ImGui::Spacing();
			return changed;
		}

		int FindGraphNodeIndex(const TerrainGraphAsset& asset, int nodeId)
		{
			for (int i = 0; i < static_cast<int>(asset.nodes.size()); ++i)
			{
				if (asset.nodes[i].id == nodeId)
				{
					return i;
				}
			}
			return -1;
		}

		bool CanConnectGraphPorts(const TerrainGraphAsset& asset, int fromNodeId, const std::string& fromPortId, int toNodeId, const std::string& toPortId)
		{
			if (fromNodeId == toNodeId)
			{
				return false;
			}
			const auto* fromNode = asset.FindNode(fromNodeId);
			const auto* toNode = asset.FindNode(toNodeId);
			if (!fromNode || !toNode)
			{
				return false;
			}
			const auto* fromType = FindTerrainGraphNodeType(fromNode->typeId);
			const auto* toType = FindTerrainGraphNodeType(toNode->typeId);
			if (!fromType || !toType)
			{
				return false;
			}
			const auto* fromPort = FindTerrainGraphOutput(*fromType, fromPortId);
			const auto* toPort = FindTerrainGraphInput(*toType, toPortId);
			return fromPort && toPort && fromPort->valueType == toPort->valueType;
		}

		bool ContainsSelectedNode(const std::vector<int>& selectedNodeIds, int nodeId)
		{
			return std::find(selectedNodeIds.begin(), selectedNodeIds.end(), nodeId) != selectedNodeIds.end();
		}

		void SelectSingleNode(std::vector<int>& selectedNodeIds, int nodeId)
		{
			selectedNodeIds.clear();
			if (nodeId != 0)
			{
				selectedNodeIds.push_back(nodeId);
			}
		}

		void ToggleSelectedNode(std::vector<int>& selectedNodeIds, int nodeId)
		{
			auto it = std::find(selectedNodeIds.begin(), selectedNodeIds.end(), nodeId);
			if (it != selectedNodeIds.end())
			{
				selectedNodeIds.erase(it);
			}
			else if (nodeId != 0)
			{
				selectedNodeIds.push_back(nodeId);
			}
		}

		void CopySelectedGraphNodes(const TerrainGraphAsset& asset)
		{
			auto& state = UniversalEditorState();
			state.clipboardNodes.clear();
			state.clipboardLinks.clear();
			std::unordered_set<int> copiedIds;
			for (const auto& node : asset.nodes)
			{
				if (ContainsSelectedNode(state.selectedNodeIds, node.id))
				{
					state.clipboardNodes.push_back(node);
					copiedIds.insert(node.id);
				}
			}
			for (const auto& link : asset.links)
			{
				if (copiedIds.contains(link.fromNodeId) && copiedIds.contains(link.toNodeId))
				{
					state.clipboardLinks.push_back(link);
				}
			}
		}

		bool DeleteSelectedGraphNodes(TerrainGraphAsset& asset, int& selectedNodeId)
		{
			auto& selectedNodeIds = UniversalEditorState().selectedNodeIds;
			if (selectedNodeIds.empty()) return false;
			const std::vector<int> idsToDelete = selectedNodeIds;
			bool removed = false;
			for (int nodeId : idsToDelete) removed = asset.RemoveNode(nodeId) || removed;
			selectedNodeIds.clear();
			selectedNodeId = asset.nodes.empty() ? 0 : asset.nodes.front().id;
			if (selectedNodeId != 0) selectedNodeIds.push_back(selectedNodeId);
			return removed;
		}

		bool PasteGraphNodes(TerrainGraphAsset& asset, int& selectedNodeId, std::optional<ImVec2> targetPosition)
		{
			auto& state = UniversalEditorState();
			if (state.clipboardNodes.empty()) return false;
			ImVec2 sourceMin = state.clipboardNodes.front().position;
			for (const auto& node : state.clipboardNodes)
			{
				sourceMin.x = std::min(sourceMin.x, node.position.x);
				sourceMin.y = std::min(sourceMin.y, node.position.y);
			}
			const ImVec2 target = targetPosition.value_or(Add(sourceMin, ImVec2(32.0f, 32.0f)));
			const ImVec2 offset = Sub(target, sourceMin);
			std::unordered_map<int, int> remappedIds;
			state.selectedNodeIds.clear();
			for (const auto& source : state.clipboardNodes)
			{
				TerrainGraphNodeInstance pasted = source;
				pasted.id = asset.nextNodeId++;
				pasted.position = Add(source.position, offset);
				remappedIds[source.id] = pasted.id;
				state.selectedNodeIds.push_back(pasted.id);
				asset.nodes.push_back(std::move(pasted));
			}
			for (const auto& source : state.clipboardLinks)
			{
				auto from = remappedIds.find(source.fromNodeId);
				auto to = remappedIds.find(source.toNodeId);
				if (from == remappedIds.end() || to == remappedIds.end()) continue;
				TerrainGraphLink pasted = source;
				pasted.id = asset.nextLinkId++;
				pasted.fromNodeId = from->second;
				pasted.toNodeId = to->second;
				asset.links.push_back(std::move(pasted));
			}
			selectedNodeId = state.selectedNodeIds.front();
			return true;
		}

		bool RectsOverlap(ImVec2 aMin, ImVec2 aMax, ImVec2 bMin, ImVec2 bMax)
		{
			return aMin.x <= bMax.x && aMax.x >= bMin.x && aMin.y <= bMax.y && aMax.y >= bMin.y;
		}

		ImVec2 Min(ImVec2 a, ImVec2 b)
		{
			return ImVec2(std::min(a.x, b.x), std::min(a.y, b.y));
		}

		ImVec2 Max(ImVec2 a, ImVec2 b)
		{
			return ImVec2(std::max(a.x, b.x), std::max(a.y, b.y));
		}

		struct GraphMaskPreview
		{
			std::string label;
			std::shared_ptr<TerrainMask> mask;
		};

		bool HasMaskPreviewSlot(const TerrainGraphNodeType& type)
		{
			return std::any_of(type.outputs.begin(), type.outputs.end(), [](const TerrainGraphPortDefinition& port)
			{
				return port.valueType == TerrainGraphValueType::FloatMask;
			}) || std::any_of(type.inputs.begin(), type.inputs.end(), [](const TerrainGraphPortDefinition& port)
			{
				return port.valueType == TerrainGraphValueType::FloatMask;
			});
		}

		float MaskPreviewTop(const TerrainGraphNodeType& type)
		{
			const size_t portRows = std::max(type.inputs.size(), type.outputs.size());
			return std::max(82.0f, 68.0f + static_cast<float>(portRows) * 18.0f);
		}

		ImVec2 UniversalNodeSize(const TerrainGraphNodeType& type, bool showPreviews)
		{
			if (showPreviews && HasMaskPreviewSlot(type))
			{
				return ImVec2(230.0f, MaskPreviewTop(type) + 224.0f);
			}
			return ImVec2(230.0f, 154.0f);
		}

		std::vector<GraphMaskPreview> FindGraphMaskPreviews(const TerrainGraphAsset& asset, const TerrainGraphNodeInstance& node,
			const TerrainGraphNodeType& type, const std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks)
		{
			std::vector<GraphMaskPreview> previews;
			for (const auto& output : type.outputs)
			{
				if (output.valueType != TerrainGraphValueType::FloatMask) continue;
				auto found = masks.find("preview_node_" + std::to_string(node.id) + "_" + output.id);
				if (found != masks.end() && found->second && found->second->texture)
				{
					previews.push_back({ output.displayName, found->second });
				}
			}
			if (!previews.empty()) return previews;

			for (const auto& input : type.inputs)
			{
				if (input.valueType != TerrainGraphValueType::FloatMask) continue;
				const TerrainGraphLink* link = asset.FindInputLink(node.id, input.id);
				if (!link) continue;
				auto found = masks.find("preview_node_" + std::to_string(link->fromNodeId) + "_" + link->fromPortId);
				if (found != masks.end() && found->second && found->second->texture)
				{
					previews.push_back({ input.displayName, found->second });
				}
			}
			return previews;
		}

		bool DrawUniversalGraphCanvas(TerrainGraphAsset& asset, int& selectedNodeId,
			const std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, bool* viewChanged, bool* graphChanged, bool readOnly = false)
		{
			auto& editorState = UniversalEditorState();
			auto& selectedNodeIds = editorState.selectedNodeIds;
			static bool selectionDragActive = false;
			static ImVec2 selectionStart;
			static ImVec2 selectionEnd;
			if (viewChanged)
			{
				*viewChanged = false;
			}
			if (graphChanged) *graphChanged = false;
			if (selectedNodeId != 0 && selectedNodeIds.empty())
			{
				SelectSingleNode(selectedNodeIds, selectedNodeId);
			}
			selectedNodeIds.erase(std::remove_if(selectedNodeIds.begin(), selectedNodeIds.end(), [&](int nodeId)
			{
				return asset.FindNode(nodeId) == nullptr;
			}), selectedNodeIds.end());
			if (!asset.FindNode(selectedNodeId)) selectedNodeId = selectedNodeIds.empty() ? 0 : selectedNodeIds.front();

			ImVec2 viewportSize = ImGui::GetContentRegionAvail();
			viewportSize.x = std::max(viewportSize.x, 360.0f);
			viewportSize.y = std::max(viewportSize.y, 280.0f);
			ImVec2 origin = ImGui::GetCursorScreenPos();
			ImGui::InvisibleButton("universal_graph_canvas", viewportSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle | ImGuiButtonFlags_AllowOverlap);
			ImVec2 viewportMax(origin.x + viewportSize.x, origin.y + viewportSize.y);
			bool hovered = ImGui::IsItemHovered() && !readOnly;
			bool canvasActive = ImGui::IsItemActive() && !readOnly;
			ImGuiIO& io = ImGui::GetIO();
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			bool capturedInput = hovered || canvasActive;
			bool hoveredNodeOrPort = false;

			if (hovered && io.MouseWheel != 0.0f)
			{
				float oldZoom = asset.zoom;
				asset.zoom = std::clamp(asset.zoom * (io.MouseWheel > 0.0f ? 1.12f : 0.89f), 0.30f, 2.75f);
				ImVec2 mouseLocal(io.MousePos.x - origin.x, io.MousePos.y - origin.y);
				asset.pan = Sub(mouseLocal, Mul(Sub(mouseLocal, asset.pan), asset.zoom / oldZoom));
				if (viewChanged) *viewChanged = true;
			}
			if ((hovered || canvasActive) && (ImGui::IsMouseDragging(ImGuiMouseButton_Right) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle)))
			{
				asset.pan.x += io.MouseDelta.x;
				asset.pan.y += io.MouseDelta.y;
				capturedInput = true;
				if (viewChanged && (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f)) *viewChanged = true;
			}

			auto toScreen = [&](ImVec2 world)
			{
				return Add(origin, Add(asset.pan, Mul(world, asset.zoom)));
			};
			auto toWorld = [&](ImVec2 screen)
			{
				return Mul(Sub(Sub(screen, origin), asset.pan), 1.0f / asset.zoom);
			};
			auto portPosition = [&](const TerrainGraphNodeInstance& node, bool output, int portIndex)
			{
				ImVec2 pos = toScreen(node.position);
				float y = pos.y + (58.0f + static_cast<float>(portIndex) * 18.0f) * asset.zoom;
				return output ? ImVec2(pos.x + 230.0f * asset.zoom, y) : ImVec2(pos.x, y);
			};

			drawList->PushClipRect(origin, viewportMax, true);
			drawList->AddRectFilled(origin, viewportMax, IM_COL32(12, 14, 16, 46));
			float gridStep = 64.0f * asset.zoom;
			if (gridStep >= 12.0f)
			{
				float startX = origin.x + std::fmod(asset.pan.x, gridStep);
				float startY = origin.y + std::fmod(asset.pan.y, gridStep);
				for (float x = startX; x < viewportMax.x; x += gridStep)
				{
					drawList->AddLine(ImVec2(x, origin.y), ImVec2(x, viewportMax.y), IM_COL32(255, 255, 255, 18));
				}
				for (float y = startY; y < viewportMax.y; y += gridStep)
				{
					drawList->AddLine(ImVec2(origin.x, y), ImVec2(viewportMax.x, y), IM_COL32(255, 255, 255, 18));
				}
			}

			int hoveredLink = -1;
			for (const auto& link : asset.links)
			{
				const auto* fromNode = asset.FindNode(link.fromNodeId);
				const auto* toNode = asset.FindNode(link.toNodeId);
				if (!fromNode || !toNode)
				{
					continue;
				}
				const auto* fromType = FindTerrainGraphNodeType(fromNode->typeId);
				const auto* toType = FindTerrainGraphNodeType(toNode->typeId);
				if (!fromType || !toType)
				{
					continue;
				}
				int fromPortIndex = -1;
				int toPortIndex = -1;
				for (int i = 0; i < static_cast<int>(fromType->outputs.size()); ++i)
				{
					if (fromType->outputs[i].id == link.fromPortId) fromPortIndex = i;
				}
				for (int i = 0; i < static_cast<int>(toType->inputs.size()); ++i)
				{
					if (toType->inputs[i].id == link.toPortId) toPortIndex = i;
				}
				if (fromPortIndex < 0 || toPortIndex < 0)
				{
					continue;
				}
				ImVec2 from = portPosition(*fromNode, true, fromPortIndex);
				ImVec2 to = portPosition(*toNode, false, toPortIndex);
				bool linkHovered = hovered && DistanceToConnectionSquared(io.MousePos, from, to) <= std::max(7.0f, 6.0f * asset.zoom) * std::max(7.0f, 6.0f * asset.zoom);
				if (linkHovered)
				{
					hoveredLink = link.id;
				}
				DrawConnection(drawList, from, to, linkHovered ? IM_COL32(235, 170, 255, 255) : IM_COL32(185, 130, 230, 220), linkHovered ? std::max(2.0f, 3.2f * asset.zoom) : std::max(1.2f, 2.2f * asset.zoom));
			}
			if (hoveredLink >= 0)
			{
				ImGui::SetTooltip("Press C to cut link");
				if (!io.WantTextInput && !io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C))
				{
					asset.RemoveLink(hoveredLink);
					if (viewChanged) *viewChanged = true;
					if (graphChanged) *graphChanged = true;
				}
			}

			static int dragNodeId = 0;
			static std::string dragPortId;
			if (dragNodeId != 0)
			{
				const auto* sourceNode = asset.FindNode(dragNodeId);
				const auto* sourceType = sourceNode ? FindTerrainGraphNodeType(sourceNode->typeId) : nullptr;
				int outputIndex = -1;
				if (sourceType)
				{
					for (int i = 0; i < static_cast<int>(sourceType->outputs.size()); ++i)
					{
						if (sourceType->outputs[i].id == dragPortId) outputIndex = i;
					}
				}
				if (sourceNode && outputIndex >= 0)
				{
					ImVec2 from = portPosition(*sourceNode, true, outputIndex);
					DrawConnection(drawList, from, io.MousePos, IM_COL32(230, 190, 90, 230), std::max(1.5f, 2.6f * asset.zoom));
					capturedInput = true;
				}
			}

			for (auto& node : asset.nodes)
			{
				const auto* type = FindTerrainGraphNodeType(node.typeId);
				if (!type)
				{
					continue;
				}
				ImVec2 pos = toScreen(node.position);
				const ImVec2 nodeSize = UniversalNodeSize(*type, editorState.showMaskPreviews);
				ImVec2 scaledNode = Mul(nodeSize, asset.zoom);
				ImVec2 max(pos.x + scaledNode.x, pos.y + scaledNode.y);
				bool selected = selectedNodeId == node.id || ContainsSelectedNode(selectedNodeIds, node.id);
				bool onScreen = max.x >= origin.x && pos.x <= viewportMax.x && max.y >= origin.y && pos.y <= viewportMax.y;
				bool portStarted = false;

				if (onScreen && hovered)
				{
					ImGui::PushID(node.id);
					for (int outputIndex = 0; outputIndex < static_cast<int>(type->outputs.size()); ++outputIndex)
					{
						ImVec2 portPos = portPosition(node, true, outputIndex);
						float hitRadius = std::max(10.0f, 9.0f * asset.zoom);
						ImGui::PushID(outputIndex);
						ImGui::SetCursorScreenPos(ImVec2(portPos.x - hitRadius, portPos.y - hitRadius));
						ImGui::SetNextItemAllowOverlap();
						ImGui::InvisibleButton("v2_output", ImVec2(hitRadius * 2.0f, hitRadius * 2.0f));
						hoveredNodeOrPort = hoveredNodeOrPort || ImGui::IsItemHovered();
						if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
						{
							dragNodeId = node.id;
							dragPortId = type->outputs[outputIndex].id;
							selectedNodeId = node.id;
							if (!ContainsSelectedNode(selectedNodeIds, node.id))
							{
								SelectSingleNode(selectedNodeIds, node.id);
							}
							portStarted = true;
							capturedInput = true;
						}
						if (ImGui::IsItemHovered())
						{
							ImGui::SetTooltip("%s (%s)", type->outputs[outputIndex].displayName.c_str(), TerrainGraphValueTypeName(type->outputs[outputIndex].valueType));
						}
						ImGui::PopID();
					}
					ImGui::PopID();
				}

				if (onScreen && !portStarted)
				{
					ImGui::SetCursorScreenPos(pos);
					ImGui::PushID(node.id);
					ImGui::SetNextItemAllowOverlap();
					ImGui::InvisibleButton("v2_node", scaledNode);
					hoveredNodeOrPort = hoveredNodeOrPort || ImGui::IsItemHovered();
					if (ImGui::IsItemClicked())
					{
						selectedNodeId = node.id;
						if (io.KeyShift || io.KeyCtrl)
						{
							ToggleSelectedNode(selectedNodeIds, node.id);
							if (selectedNodeIds.empty())
							{
								selectedNodeId = 0;
							}
						}
						else if (!ContainsSelectedNode(selectedNodeIds, node.id))
						{
							SelectSingleNode(selectedNodeIds, node.id);
						}
					}
					if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
					{
						if (ContainsSelectedNode(selectedNodeIds, node.id) && selectedNodeIds.size() > 1)
						{
							std::unordered_set<int> moved;
							for (int selectedId : selectedNodeIds)
							{
								if (moved.contains(selectedId))
								{
									continue;
								}
								if (auto* selectedNode = asset.FindNode(selectedId))
								{
									selectedNode->position.x += io.MouseDelta.x / asset.zoom;
									selectedNode->position.y += io.MouseDelta.y / asset.zoom;
									moved.insert(selectedId);
								}
							}
						}
						else
						{
							node.position.x += io.MouseDelta.x / asset.zoom;
							node.position.y += io.MouseDelta.y / asset.zoom;
						}
						capturedInput = true;
						if (viewChanged && (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f)) *viewChanged = true;
					}
					ImGui::PopID();
				}

				ImU32 accent = node.enabled ? IM_COL32(80, 168, 130, 255) : IM_COL32(110, 110, 110, 255);
				drawList->AddRectFilled(pos, max, selected ? IM_COL32(38, 45, 50, 255) : IM_COL32(28, 31, 34, 255), 4.0f * asset.zoom);
				drawList->AddRect(pos, max, selected ? accent : IM_COL32(64, 70, 76, 255), 4.0f * asset.zoom, 0, selected ? 2.0f : 1.0f);
				drawList->AddRectFilled(pos, ImVec2(max.x, pos.y + 5.0f * asset.zoom), accent, 4.0f * asset.zoom);
				float fontSize = ImGui::GetFontSize() * std::clamp(asset.zoom, 0.55f, 1.25f);
				drawList->AddText(ImGui::GetFont(), fontSize, ImVec2(pos.x + 10.0f * asset.zoom, pos.y + 18.0f * asset.zoom), IM_COL32(235, 238, 240, 255), node.displayName.c_str());
				drawList->AddText(ImGui::GetFont(), fontSize, ImVec2(pos.x + 10.0f * asset.zoom, pos.y + 40.0f * asset.zoom), IM_COL32(150, 155, 164, 230), type->category.c_str());

				if (editorState.showMaskPreviews && HasMaskPreviewSlot(*type))
				{
					const float previewTop = MaskPreviewTop(*type);
					ImVec2 previewMin(pos.x + 10.0f * asset.zoom, pos.y + previewTop * asset.zoom);
					ImVec2 previewMax(pos.x + 220.0f * asset.zoom, pos.y + (previewTop + 210.0f) * asset.zoom);
					drawList->AddRectFilled(previewMin, previewMax, IM_COL32(8, 10, 12, 255), 2.0f * asset.zoom);
					auto previews = FindGraphMaskPreviews(asset, node, *type, masks);
					if (!previews.empty())
					{
						drawList->AddImage((ImTextureID)(intptr_t)previews.front().mask->texture->GetTextureID(),
							previewMin, previewMax, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
					}
					else if (asset.zoom >= 0.55f)
					{
						drawList->AddText(ImGui::GetFont(), fontSize * 0.82f,
							ImVec2(previewMin.x + 8.0f * asset.zoom, previewMin.y + 95.0f * asset.zoom),
							IM_COL32(125, 132, 140, 220), "Run Graph to preview");
					}
					drawList->AddRect(previewMin, previewMax, IM_COL32(78, 86, 94, 220), 2.0f * asset.zoom);
				}

				float portRadius = std::max(3.0f, 4.0f * asset.zoom);
				float portFontSize = ImGui::GetFontSize() * std::clamp(asset.zoom, 0.45f, 0.90f);
				for (int i = 0; i < static_cast<int>(type->inputs.size()); ++i)
				{
					ImVec2 portPos = portPosition(node, false, i);
					bool compatible = dragNodeId != 0 && CanConnectGraphPorts(asset, dragNodeId, dragPortId, node.id, type->inputs[i].id);
					drawList->AddCircleFilled(portPos, portRadius, IM_COL32(92, 150, 220, 255));
					if (compatible)
					{
						drawList->AddCircle(portPos, portRadius + 4.0f * asset.zoom, IM_COL32(112, 220, 150, 240), 16, 1.8f);
					}
					if (asset.zoom >= 0.48f)
					{
						drawList->AddText(ImGui::GetFont(), portFontSize, ImVec2(portPos.x + 9.0f * asset.zoom, portPos.y - 7.0f * asset.zoom), IM_COL32(165, 190, 220, 220), type->inputs[i].displayName.c_str());
					}
				}
				for (int i = 0; i < static_cast<int>(type->outputs.size()); ++i)
				{
					ImVec2 portPos = portPosition(node, true, i);
					drawList->AddCircleFilled(portPos, portRadius, IM_COL32(92, 190, 130, 255));
					if (asset.zoom >= 0.48f)
					{
						ImVec2 textSize = ImGui::CalcTextSize(type->outputs[i].displayName.c_str());
						drawList->AddText(ImGui::GetFont(), portFontSize, ImVec2(portPos.x - textSize.x * (portFontSize / ImGui::GetFontSize()) - 10.0f * asset.zoom, portPos.y - 7.0f * asset.zoom), IM_COL32(170, 220, 185, 220), type->outputs[i].displayName.c_str());
					}
				}
			}

			if ((hovered || canvasActive) && !hoveredNodeOrPort && dragNodeId == 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				selectionDragActive = true;
				selectionStart = io.MousePos;
				selectionEnd = io.MousePos;
				if (!io.KeyShift && !io.KeyCtrl)
				{
					selectedNodeIds.clear();
					selectedNodeId = 0;
				}
			}
			if (selectionDragActive)
			{
				selectionEnd = io.MousePos;
				capturedInput = true;
				ImVec2 rectMin = Min(selectionStart, selectionEnd);
				ImVec2 rectMax = Max(selectionStart, selectionEnd);
				drawList->AddRectFilled(rectMin, rectMax, IM_COL32(90, 150, 220, 42));
				drawList->AddRect(rectMin, rectMax, IM_COL32(120, 180, 245, 230), 0.0f, 0, 1.5f);
				if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
				{
					for (const auto& node : asset.nodes)
					{
						const auto* type = FindTerrainGraphNodeType(node.typeId);
						if (!type) continue;
						ImVec2 nodeMin = toScreen(node.position);
						ImVec2 nodeMax = Add(nodeMin, Mul(UniversalNodeSize(*type, editorState.showMaskPreviews), asset.zoom));
						if (RectsOverlap(rectMin, rectMax, nodeMin, nodeMax) && !ContainsSelectedNode(selectedNodeIds, node.id))
						{
							selectedNodeIds.push_back(node.id);
						}
					}
					selectedNodeId = selectedNodeIds.empty() ? 0 : selectedNodeIds.front();
					selectionDragActive = false;
					if (viewChanged)
					{
						*viewChanged = true;
					}
				}
			}

			if (dragNodeId != 0 && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				for (const auto& node : asset.nodes)
				{
					const auto* type = FindTerrainGraphNodeType(node.typeId);
					if (!type)
					{
						continue;
					}
					for (int i = 0; i < static_cast<int>(type->inputs.size()); ++i)
					{
						ImVec2 portPos = portPosition(node, false, i);
						float hitRadius = std::max(12.0f, 10.0f * asset.zoom);
						if (DistanceSquared(io.MousePos, portPos) <= hitRadius * hitRadius && CanConnectGraphPorts(asset, dragNodeId, dragPortId, node.id, type->inputs[i].id))
						{
							if (asset.AddLink(dragNodeId, dragPortId, node.id, type->inputs[i].id))
							{
								if (viewChanged) *viewChanged = true;
								if (graphChanged) *graphChanged = true;
							}
							selectedNodeId = node.id;
						}
					}
				}
				dragNodeId = 0;
				dragPortId.clear();
			}

			const bool acceptsShortcuts = !readOnly && (hovered || canvasActive || ImGui::IsWindowFocused()) && !io.WantTextInput;
			bool shortcutChanged = false;
			if (acceptsShortcuts)
			{
				if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A))
				{
					selectedNodeIds.clear();
					for (const auto& node : asset.nodes) selectedNodeIds.push_back(node.id);
					selectedNodeId = selectedNodeIds.empty() ? 0 : selectedNodeIds.front();
				}
				else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C))
				{
					CopySelectedGraphNodes(asset);
				}
				else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_X))
				{
					CopySelectedGraphNodes(asset);
					shortcutChanged = DeleteSelectedGraphNodes(asset, selectedNodeId);
				}
				else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V))
				{
					std::optional<ImVec2> pastePosition;
					if (hovered) pastePosition = toWorld(io.MousePos);
					shortcutChanged = PasteGraphNodes(asset, selectedNodeId, pastePosition);
				}
				else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D))
				{
					CopySelectedGraphNodes(asset);
					shortcutChanged = PasteGraphNodes(asset, selectedNodeId, std::nullopt);
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace))
				{
					shortcutChanged = DeleteSelectedGraphNodes(asset, selectedNodeId);
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
				{
					selectedNodeIds.clear();
					selectedNodeId = 0;
					dragNodeId = 0;
					dragPortId.clear();
				}
			}
			if (shortcutChanged)
			{
				selectedNodeIds.erase(std::remove_if(selectedNodeIds.begin(), selectedNodeIds.end(), [&](int nodeId)
				{
					return asset.FindNode(nodeId) == nullptr;
				}), selectedNodeIds.end());
				if (!asset.FindNode(selectedNodeId)) selectedNodeId = selectedNodeIds.empty() ? 0 : selectedNodeIds.front();
				if (viewChanged) *viewChanged = true;
				if (graphChanged) *graphChanged = true;
			}

			drawList->AddText(ImVec2(origin.x + 10.0f, viewportMax.y - 24.0f), IM_COL32(190, 195, 200, 210), ("Zoom " + std::to_string(static_cast<int>(asset.zoom * 100.0f)) + "%").c_str());
			std::string selectionLabel = "Selected " + std::to_string(static_cast<int>(selectedNodeIds.size()));
			drawList->AddText(ImVec2(origin.x + 100.0f, viewportMax.y - 24.0f), IM_COL32(170, 178, 188, 210), selectionLabel.c_str());
			drawList->AddText(ImVec2(origin.x + 190.0f, viewportMax.y - 24.0f), IM_COL32(140, 145, 150, 190), "Ctrl+C/V copy/paste | Ctrl+D duplicate | Delete removes | Ctrl+Z/Y undo/redo");
			drawList->PopClipRect();
			return capturedInput;
		}

		bool DrawUniversalGraphInspector(TerrainGraphAsset& asset, int& selectedNodeId, const std::shared_ptr<VegetationAssetRegistry>& assetRegistry,
			const std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, bool* viewChanged)
		{
			bool changed = false;
			TerrainGraphNodeInstance* node = asset.FindNode(selectedNodeId);
			if (!node)
			{
				ImGui::TextDisabled("Select a universal graph node.");
				return false;
			}
			const auto* type = FindTerrainGraphNodeType(node->typeId);
			if (!type)
			{
				ImGui::TextDisabled("Unknown node type.");
				return false;
			}
			TerrainGraphParameter* assetParameter = FindNodeParameter(*node, "assetId");
			std::string previousAssetId = assetParameter ? assetParameter->stringValue : std::string{};
			const VegetationAssetDefinition* previousAsset = assetRegistry && assetParameter ? assetRegistry->Find(previousAssetId) : nullptr;

			ImGui::Text("%s", node->displayName.c_str());
			ImGui::TextDisabled("%s / %s", type->category.c_str(), type->typeId.c_str());
			ImGui::Separator();
			auto previews = FindGraphMaskPreviews(asset, *node, *type, masks);
			if (HasMaskPreviewSlot(*type))
			{
				if (!previews.empty())
				{
					for (const auto& preview : previews)
					{
						ImGui::TextDisabled("%s preview", preview.label.c_str());
						float previewSize = std::clamp(ImGui::GetContentRegionAvail().x, 160.0f, 320.0f);
						ImGui::Image((ImTextureID)(intptr_t)preview.mask->texture->GetTextureID(),
							ImVec2(previewSize, previewSize), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
					}
				}
				else
				{
					ImGui::TextDisabled("Run Graph to generate preview.");
				}
				ImGui::Separator();
			}
			changed |= ImGui::Checkbox("Enabled", &node->enabled);
			char nameBuffer[96]{};
			std::snprintf(nameBuffer, sizeof(nameBuffer), "%s", node->displayName.c_str());
			if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
			{
				node->displayName = nameBuffer;
				changed = true;
			}

			ImGui::BeginDisabled(node->typeId == "particleErosion");
			if (ImGui::Button("Duplicate"))
			{
				TerrainGraphNodeInstance duplicate = *node;
				duplicate.id = asset.nextNodeId++;
				duplicate.displayName += " Copy";
				duplicate.position.x += 32.0f;
				duplicate.position.y += 32.0f;
				asset.nodes.push_back(std::move(duplicate));
				selectedNodeId = asset.nodes.back().id;
				SelectSingleNode(UniversalEditorState().selectedNodeIds, selectedNodeId);
				changed = true;
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("Delete"))
			{
				asset.RemoveNode(node->id);
				selectedNodeId = asset.nodes.empty() ? 0 : asset.nodes.front().id;
				SelectSingleNode(UniversalEditorState().selectedNodeIds, selectedNodeId);
				changed = true;
				if (viewChanged) *viewChanged = true;
				return true;
			}

			ImGui::Spacing();
			if (ImGui::CollapsingHeader("Ports", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::TextDisabled("Inputs");
				for (const auto& input : type->inputs)
				{
					ImGui::BulletText("%s (%s)%s", input.displayName.c_str(), TerrainGraphValueTypeName(input.valueType), input.required ? "" : " optional");
				}
				ImGui::TextDisabled("Outputs");
				for (const auto& output : type->outputs)
				{
					ImGui::BulletText("%s (%s)", output.displayName.c_str(), TerrainGraphValueTypeName(output.valueType));
				}
			}

			if (ImGui::CollapsingHeader("Parameters", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (node->parameters.empty())
				{
					ImGui::TextDisabled("No parameters.");
				}
				ImGui::PushItemWidth(-1.0f);
				for (auto& param : node->parameters)
				{
					ImGui::PushID(param.id.c_str());
					changed |= DrawGraphParameterControl(param, assetRegistry);
					ImGui::PopID();
				}
				ImGui::PopItemWidth();
			}

			if (assetParameter && assetParameter->stringValue != previousAssetId && assetRegistry)
			{
				const VegetationAssetDefinition* selectedAsset = assetRegistry->Find(assetParameter->stringValue);
				if (selectedAsset && (!previousAsset || selectedAsset->kind != previousAsset->kind))
				{
					auto setFloat = [&](const char* id, float value)
					{
						if (TerrainGraphParameter* parameter = FindNodeParameter(*node, id)) parameter->floatValue = value;
					};
					auto setBool = [&](const char* id, bool value)
					{
						if (TerrainGraphParameter* parameter = FindNodeParameter(*node, id)) parameter->boolValue = value;
					};
					if (selectedAsset->kind == VegetationKind::Grass)
					{
						setFloat("density", 0.95f);
						setFloat("spacing", 3.0f);
						setFloat("minScale", 0.75f);
						setFloat("maxScale", 1.35f);
						setFloat("maxSlope", 38.0f);
						setBool("alignToSlope", false);
						setFloat("maxTilt", 30.0f);
						setFloat("renderDistance", 180.0f);
					}
					else if (selectedAsset->kind == VegetationKind::Rock)
					{
						setFloat("density", 0.55f);
						setFloat("spacing", 14.0f);
						setFloat("minScale", 0.70f);
						setFloat("maxScale", 1.40f);
						setFloat("maxSlope", 30.0f);
						setBool("alignToSlope", true);
						setFloat("maxTilt", 35.0f);
						setFloat("renderDistance", 1000.0f);
					}
					else
					{
						setFloat("density", 0.45f);
						setFloat("spacing", 55.0f);
						setFloat("minScale", 0.80f);
						setFloat("maxScale", 1.25f);
						setFloat("maxSlope", 15.0f);
						setBool("alignToSlope", false);
						setFloat("maxTilt", 15.0f);
						setFloat("renderDistance", 1400.0f);
					}
				}
			}

			if (changed && viewChanged)
			{
				*viewChanged = true;
			}
			return changed;
		}
	}

	void TerrainGraphEditor::ResetHistory()
	{
		UniversalEditorState() = UniversalGraphEditorState{};
	}

	bool TerrainGraphEditor::Show(std::shared_ptr<ChunkManager> chunkManager, TerrainGraphViewState& viewState, std::shared_ptr<VegetationAssetRegistry> assetRegistry, bool* inputCaptured, bool* viewChanged)
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
		static int selectedUniversalNodeId = 0;
		bool changed = false;
		const bool erosionRunning = chunkManager->IsParticleErosionActive();
		const bool hydrologyRunning = chunkManager->IsHydrologyRunning();
		const bool graphLocked = erosionRunning || hydrologyRunning;

		ImGui::Text("TerrainGraph");
		ImGui::SameLine();
		ImGui::TextDisabled("%d legacy passes | universal graph v2", static_cast<int>(passes.size()));
		ImGui::Separator();

		struct TerrainPreset
		{
			const char* label;
			int chunks;
			int chunkSize;
		};
		static const TerrainPreset terrainPresets[] =
		{
			{ "Small - 9 x 65", 9, 65 },
			{ "Medium - 15 x 97", 15, 97 },
			{ "Large - 31 x 129", 31, 129 },
			{ "Wide Detail - 15 x 129", 15, 129 },
			{ "Wide - 21 x 129", 21, 129 }
		};
		static const char* terrainPresetLabels[] =
		{
			"Small - 9 x 65",
			"Medium - 15 x 97",
			"Large - 31 x 129",
			"Wide Detail - 15 x 129",
			"Wide - 21 x 129"
		};
		int currentPreset = -1;
		for (int i = 0; i < static_cast<int>(IM_ARRAYSIZE(terrainPresets)); ++i)
		{
			if (chunkManager->GetChunkCount() == terrainPresets[i].chunks && chunkManager->GetChunkSize() == terrainPresets[i].chunkSize)
			{
				currentPreset = i;
				break;
			}
		}
		int selectedPreset = std::max(currentPreset, 0);
		ImGui::SetNextItemWidth(190.0f);
		ImGui::BeginDisabled(graphLocked);
		const bool terrainSizeChanged = ImGui::Combo("Terrain size", &selectedPreset, terrainPresetLabels, IM_ARRAYSIZE(terrainPresetLabels));
		ImGui::EndDisabled();
		if (terrainSizeChanged)
		{
			const auto& preset = terrainPresets[selectedPreset];
			chunkManager->ConfigureTerrain(preset.chunks, preset.chunkSize);
			terrainComputePipeline = chunkManager->GetTerrainComputePipeline();
			if (!terrainComputePipeline || !terrainComputePipeline->GetGraph())
			{
				ImGui::TextDisabled("Terrain graph is not available after resize.");
				return true;
			}
			changed = true;
			if (viewChanged)
			{
				*viewChanged = true;
			}
			return true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("World: %d | chunks: %d | chunk size: %d", chunkManager->GetWorldSize(), chunkManager->GetChunkCount(), chunkManager->GetChunkSize());
		ImGui::TextDisabled("Hydrology");
		ImGui::BeginDisabled(hydrologyRunning || erosionRunning);
		if (ImGui::Button("Generate Hydrology"))
		{
			chunkManager->GenerateHydrology();
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset Hydrology"))
		{
			chunkManager->ResetHydrology();
			changed = true;
		}
		ImGui::EndDisabled();
		if (hydrologyRunning)
		{
			ImGui::SameLine();
			if (ImGui::Button("Cancel Hydrology")) chunkManager->CancelHydrology();
			ImGui::SameLine();
			ImGui::ProgressBar(chunkManager->GetHydrologyProgress(), ImVec2(150.0f, 0.0f));
		}
		ImGui::SameLine();
		ImGui::TextDisabled("%s", chunkManager->GetHydrologyStatus().c_str());
		ImGui::Separator();
		ImGui::TextDisabled("Particle Erosion");
		const bool erosionReady = chunkManager->IsParticleErosionReady();
		ImGui::BeginDisabled(!erosionReady);
		if (ImGui::Button(erosionRunning ? "Pause Erosion" : "Start Erosion"))
		{
			if (erosionRunning) chunkManager->PauseParticleErosion();
			else chunkManager->StartParticleErosion();
			changed = true;
		}
		ImGui::SameLine();
		ImGui::BeginDisabled(erosionRunning);
		if (ImGui::Button("Step Erosion"))
		{
			changed = chunkManager->StepParticleErosion() || changed;
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset Erosion"))
		{
			changed = chunkManager->ResetParticleErosion() || changed;
		}
		ImGui::EndDisabled();
		ImGui::NewLine();
		float erosionSpeed = chunkManager->GetParticleErosionSpeed();
		ImGui::SetNextItemWidth(130.0f);
		if (ImGui::SliderFloat("Simulation Speed", &erosionSpeed, 0.25f, 4.0f, "%.2fx"))
		{
			chunkManager->SetParticleErosionSpeed(erosionSpeed);
		}
		ImGui::SameLine();
		bool showParticles = chunkManager->GetShowErosionParticles();
		if (ImGui::Checkbox("Show Particles", &showParticles))
		{
			chunkManager->SetShowErosionParticles(showParticles);
		}
		ImGui::EndDisabled();
		if (erosionReady)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("%d particles", chunkManager->GetActiveParticleCount());
		}
		ImGui::NewLine();
		ImGui::BeginDisabled(erosionRunning);
		if (ImGui::Button("Spawn/Refresh Assets"))
		{
			changed = chunkManager->RefreshProceduralAssets() || changed;
		}
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rebuild procedural objects without changing terrain, water, or erosion");
		ImGui::SameLine();
		ImGui::TextDisabled("%s", chunkManager->GetProceduralAssetStatus().c_str());
		ImGui::Separator();

		if (ImGui::BeginTabBar("TerrainGraphEditorTabs"))
		{
			if (ImGui::BeginTabItem("Universal Graph"))
			{
				auto& asset = terrainComputePipeline->GetUniversalGraphAsset();
				const bool erosionActive = graphLocked;
				ImGui::BeginDisabled(erosionActive);
				EnsureGraphHistory(asset);
				bool universalViewChanged = false;
				bool universalCapturedInput = false;
				ImGuiIO& graphIo = ImGui::GetIO();
				const bool graphHasKeyboardFocus = !erosionActive && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !graphIo.WantTextInput;
				const bool undoShortcut = graphHasKeyboardFocus && graphIo.KeyCtrl && !graphIo.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z);
				const bool redoShortcut = graphHasKeyboardFocus && graphIo.KeyCtrl &&
					(ImGui::IsKeyPressed(ImGuiKey_Y) || (graphIo.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)));
				if ((undoShortcut && UndoGraph(asset)) || (redoShortcut && RedoGraph(asset)))
				{
					universalViewChanged = true;
					changed = true;
				}
				bool useUniversal = asset.useUniversalGraph;
				if (ImGui::Checkbox("Use universal graph for regeneration", &useUniversal))
				{
					asset.useUniversalGraph = useUniversal;
					universalViewChanged = true;
				}
				ImGui::SameLine();
				auto& editorState = UniversalEditorState();
				ImGui::BeginDisabled(editorState.undoStack.empty() && !editorState.transactionActive);
				if (ImGui::Button("Undo"))
				{
					if (UndoGraph(asset))
					{
						universalViewChanged = true;
						changed = true;
					}
				}
				ImGui::EndDisabled();
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Undo graph edit (Ctrl+Z)");
				ImGui::SameLine();
				ImGui::BeginDisabled(editorState.redoStack.empty());
				if (ImGui::Button("Redo"))
				{
					if (RedoGraph(asset))
					{
						universalViewChanged = true;
						changed = true;
					}
				}
				ImGui::EndDisabled();
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Redo graph edit (Ctrl+Y or Ctrl+Shift+Z)");
				ImGui::SameLine();
				if (ImGui::Button("Add Node"))
				{
					ImGui::OpenPopup("AddUniversalTerrainNode");
				}
				if (ImGui::BeginPopup("AddUniversalTerrainNode"))
				{
					const auto& types = GetTerrainGraphNodeTypes();
					bool hasParticleErosion = std::any_of(asset.nodes.begin(), asset.nodes.end(), [](const TerrainGraphNodeInstance& node)
					{
						return node.typeId == "particleErosion";
					});
					std::string currentCategory;
					for (const auto& type : types)
					{
						if (type.category != currentCategory)
						{
							if (!currentCategory.empty())
							{
								ImGui::Separator();
							}
							currentCategory = type.category;
							ImGui::TextDisabled("%s", currentCategory.c_str());
						}
						bool unavailable = type.typeId == "particleErosion" && hasParticleErosion;
						ImGui::BeginDisabled(unavailable);
						if (ImGui::Selectable(type.displayName.c_str()))
						{
							ImVec2 available = ImGui::GetContentRegionAvail();
							ImVec2 position((-asset.pan.x + available.x * 0.45f) / asset.zoom, (-asset.pan.y + available.y * 0.35f) / asset.zoom);
							auto& node = asset.AddNode(type, position);
							selectedUniversalNodeId = node.id;
							SelectSingleNode(editorState.selectedNodeIds, node.id);
							universalViewChanged = true;
						}
						ImGui::EndDisabled();
					}
					ImGui::EndPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Validate Graph"))
				{
					auto validation = ValidateTerrainGraphAsset(asset);
					for (const auto& message : validation.messages)
					{
						PWG_INFO("TerrainGraphV2: {0}", message);
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Run Graph"))
				{
					changed = chunkManager->RunUniversalGraph() || changed;
				}
				ImGui::SameLine();
				if (ImGui::Button("Run + Apply To Terrain"))
				{
					asset.useUniversalGraph = true;
					changed = chunkManager->RunUniversalGraph() || changed;
					universalViewChanged = true;
				}
				ImGui::SameLine();
				if (ImGui::Button("Reset To Default"))
				{
					terrainComputePipeline->ResetUniversalGraphToDefault();
					auto& resetAsset = terrainComputePipeline->GetUniversalGraphAsset();
					selectedUniversalNodeId = resetAsset.nodes.empty() ? 0 : resetAsset.nodes.front().id;
					SelectSingleNode(editorState.selectedNodeIds, selectedUniversalNodeId);
					universalViewChanged = true;
				}

				auto validation = ValidateTerrainGraphAsset(asset);
				ImGui::TextColored(validation.valid ? ImVec4(0.45f, 0.86f, 0.58f, 1.0f) : ImVec4(0.95f, 0.46f, 0.42f, 1.0f),
					validation.valid ? "Valid graph" : "Graph needs attention");
				ImGui::SameLine();
				ImGui::TextDisabled("%d nodes | %d links", static_cast<int>(asset.nodes.size()), static_cast<int>(asset.links.size()));
				ImGui::SameLine();
				ImGui::Checkbox("Previews", &editorState.showMaskPreviews);

				ImVec2 available = ImGui::GetContentRegionAvail();
				float detailsWidth = std::clamp(available.x * 0.34f, 260.0f, 380.0f);
				float graphWidth = std::max(available.x - detailsWidth - 8.0f, 340.0f);
				ImGui::BeginChild("UniversalTerrainGraphCanvas", ImVec2(graphWidth, 0.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
				bool canvasViewChanged = false;
				bool canvasGraphChanged = false;
				const auto& graphMasks = chunkManager->GetMasks();
				universalCapturedInput = DrawUniversalGraphCanvas(asset, selectedUniversalNodeId, graphMasks, &canvasViewChanged, &canvasGraphChanged, erosionActive);
				universalViewChanged = universalViewChanged || canvasViewChanged;
				changed = changed || canvasGraphChanged;
				ImGui::EndChild();
				ImGui::SameLine();
				ImGui::BeginChild("UniversalTerrainGraphInspector", ImVec2(0.0f, 0.0f), true);
				if (!validation.messages.empty() && ImGui::CollapsingHeader("Validation", ImGuiTreeNodeFlags_DefaultOpen))
				{
					for (const auto& message : validation.messages)
					{
						ImGui::BulletText("%s", message.c_str());
					}
				}
				bool inspectorChanged = DrawUniversalGraphInspector(asset, selectedUniversalNodeId, assetRegistry, graphMasks, &universalViewChanged);
				changed = changed || inspectorChanged;
				ImGui::EndChild();
				const bool graphInteractionActive = ImGui::IsAnyItemActive() || ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
					ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle);
				RecordGraphHistory(asset, graphInteractionActive);
				if (inputCaptured)
				{
					*inputCaptured = universalCapturedInput;
				}
				if (viewChanged)
				{
					*viewChanged = universalViewChanged;
				}
				ImGui::EndDisabled();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Legacy Passes"))
			{
				ImGui::BeginDisabled(erosionRunning);
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
				ImGui::TextDisabled("Legacy links are visual proposals until migrated.");

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
				changed = DrawInspector(chunkManager, model, passes, selectedPassIndex, viewState, &inspectorViewChanged) || changed;
				ImGui::EndChild();
				if (viewChanged)
				{
					*viewChanged = canvasViewChanged || inspectorViewChanged;
				}
				ImGui::EndDisabled();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		return changed;
	}
}
