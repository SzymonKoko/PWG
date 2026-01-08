#ifndef SRC_SCENE_CONTROLS_TERRAIN_LAYER_CONTROLS_H
#define SRC_SCENE_CONTROLS_TERRAIN_LAYER_CONTROLS_H

#include "terrain/terrain.h"
#include <thread>
#include "imgui.h"

namespace pwg::controls
{
    class TerrainLayerControls
    {
    public:

        static void ShowControl(std::vector<TerrainLayer>& terrainLayers)
        {
            static const char* textureNames[4] = { "Grass", "Dirt", "Stone", "Snow" };
            //auto& terrainLayers = terrainLayers;
            if (ImGui::BeginTabItem("TerrainLayers"))
            {
                for (auto& layer : terrainLayers)
                {
                    if (ImGui::CollapsingHeader(textureNames[layer.layerID]), ImGuiTreeNodeFlags_DefaultOpen)
                    {
                        ImGui::PushID(textureNames[layer.layerID]);
                        ImGui::Text("Name: %s", textureNames[layer.layerID]);
                        ImGui::SliderFloat("Min Height", &layer.minHeight, 0.0f, 1.0f); { layer.dirty = true; }
                        ImGui::SliderFloat("Max Height", &layer.maxHeight, 0.0f, 1.0f); { layer.dirty = true; }
                        ImGui::SliderFloat("Min Slope", &layer.minSlope, 0.0f, 1.0f); { layer.dirty = true; }
                        ImGui::SliderFloat("Max Slope", &layer.maxSlope, 0.0f, 1.0f); { layer.dirty = true; }
                        ImGui::SliderFloat("Blend Strength", &layer.blendStrength, 0.0f, 1.0f); { layer.dirty = true; }
                        ImGui::PopID();
                    }
                }

                ImGui::EndTabItem();
            }
        }
    };
}

#endif // !SRC_SCENE_CONTROLS_TERRAIN_LAYER_CONTROLS_H
