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

        static void ShowControl(Terrain& terrain)
        {
            
            auto& terrainLayers = terrain.GetTerrainLayers();
            if (ImGui::BeginTabItem("TerrainLayers"))
            {
                for (auto& layer : terrainLayers)
                {
                    if (ImGui::CollapsingHeader(layer.name.c_str()), ImGuiTreeNodeFlags_DefaultOpen)
                    {
                        ImGui::PushID(layer.name.c_str());
                        ImGui::Text("Name: %s", layer.name);
                        ImGui::Checkbox("Enabled", &layer.enabled);
                        ImGui::BeginDisabled(!layer.enabled);
                        ImGui::SliderFloat("Min Height", &layer.minHeight, 0.0f, 100.0f);
                        ImGui::SliderFloat("Max Height", &layer.maxHeight, 0.0f, 100.0f);
                        ImGui::ColorEdit3("Color", &layer.color[0]);
                        ImGui::EndDisabled();
                        ImGui::PopID();
                    }
                }

                static char newLayerNameBuffer[64] = "";
                ImGui::Separator();

                ImGui::InputTextWithHint("##LayerName", "Layer name", newLayerNameBuffer, IM_ARRAYSIZE(newLayerNameBuffer));

                ImGui::SameLine();

                if (ImGui::Button("Add layer"))
                {
                    std::string layerName = newLayerNameBuffer;
                    memset(newLayerNameBuffer, 0, sizeof(newLayerNameBuffer));
                    
                    terrain.AddLayer(pwg::TerrainLayer(true, layerName, 1, -0.0f, -0.0f, 10, glm::vec3(0.0f, 0.0f, 1.0f)));
                    
                }
                ImGui::Separator();

                ImGui::EndTabItem();
            }
        }
    };
}

#endif // !SRC_SCENE_CONTROLS_TERRAIN_LAYER_CONTROLS_H
