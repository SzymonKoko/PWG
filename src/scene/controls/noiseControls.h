#ifndef SRC_SCENE_CONTROLS_NOISE_CONTROLS_H
#define SRC_SCENE_CONTROLS_NOISE_CONTROLS_H

#include "imgui.h"
#include "FastNoiseLite.h"
#include <algorithm>

namespace pwg::controls
{
	class NoiseControls
	{
	public:

		static void ShowControl(std::shared_ptr<ChunkManager> chunkManager)
		{

            static const char* noiseTypes[] = { "OpenSimplex2", "OpenSimplex2S", "Cellular", "Perlin", "Value Cubic", "Value" };
            static const char* fractalTypes[] = { "None", "FBm", "Ridged", "Ping Pong" };
            static const char* terrainPresets[] = {
                "Small 9 x 65",
                "Medium 15 x 97",
                "Default 31 x 129",
                "Dense 15 x 129",
                "Huge 21 x 129"
            };
            static int terrainPreset = 2;
            bool updated = false;
            
            auto terrainComputePipeline = chunkManager->GetTerrainComputePipeline();


            if (ImGui::BeginTabItem("Noise"))
            {
                auto& passes = terrainComputePipeline->GetGraph()->GetPasses();

                if (ImGui::Button("Regenerate Terrain"))
                {
                    chunkManager->Regenerate();
                }
                ImGui::SameLine();
                if (ImGui::Button("Generate Hydrology")) chunkManager->GenerateHydrology();
                ImGui::SameLine();
                if (ImGui::Button("Reset Hydrology")) chunkManager->ResetHydrology();
                if (ImGui::Combo("Terrain preset", &terrainPreset, terrainPresets, IM_ARRAYSIZE(terrainPresets)))
                {
                    switch (terrainPreset)
                    {
                    case 0:
                        chunkManager->ConfigureTerrain(9, 65);
                        break;
                    case 1:
                        chunkManager->ConfigureTerrain(15, 97);
                        break;
                    case 2:
                        chunkManager->ConfigureTerrain(31, 129);
                        break;
                    case 3:
                        chunkManager->ConfigureTerrain(15, 129);
                        break;
                    case 4:
                        chunkManager->ConfigureTerrain(21, 129);
                        break;
                    default:
                        break;
                    }
                }
                ImGui::Separator();

                auto& hydraulicSettings = terrainComputePipeline->GetHydraulicSettings();
                ImGui::Text("Generated Hydrology");
                ImGui::Checkbox("Show water surface", &hydraulicSettings.showWaterSurface);
                ImGui::Checkbox("Enable water refraction", &hydraulicSettings.waterRefractionEnabled);
                ImGui::SliderFloat("Minimum visible water depth", &hydraulicSettings.minVisibleWaterDepth, 0.005f, 0.25f, "%.3f");

                ImGui::Separator();

                bool hydrologyHeaderShown = false;
                for (const auto& pass : passes)
                {
                    if (pass->GetPassType() == TerrainPassTypes::HYDROLOGY && !hydrologyHeaderShown)
                    {
                        ImGui::Separator();
                        ImGui::Text("Water Physics");
                        hydrologyHeaderShown = true;
                    }

                    ImGui::CollapsingHeader(pass->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen);

                    auto params = pass->GetParameters();

                    ImGui::PushID(pass->GetName().c_str());
                    for (auto& param : params)
                    {
                        switch (param.type)
                        {

                        case ParameterType::Float:
                        {
                            if (ImGui::SliderFloat(param.name.c_str(), (float*)param.data, param.min, param.max))
                            {
                                updated = updated || pass->GetPassType() != TerrainPassTypes::HYDROLOGY;
                            }
                            break;
                        }
                        case ParameterType::Int:
                        {
                            if (ImGui::SliderInt(param.name.c_str(), (int*)param.data, param.min, param.max))
                            {
                                updated = updated || pass->GetPassType() != TerrainPassTypes::HYDROLOGY;
                            }
                            break;
                        }
                        case ParameterType::Bool:
                        {
                            if (ImGui::Checkbox(param.name.c_str(), (bool*)param.data))
                            {
                                updated = updated || pass->GetPassType() != TerrainPassTypes::HYDROLOGY;
                            }
                            break;
                        }
                        case ParameterType::List:
                        {
                            if (param.name == "Noise Type")
                            {
                                if (ImGui::Combo(param.name.c_str(), (int*)param.data, noiseTypes, IM_ARRAYSIZE(noiseTypes)))
                                {
                                    updated = updated || pass->GetPassType() != TerrainPassTypes::HYDROLOGY;
                                }
                                break;
                            }
                            else if (param.name == "Fractal Type")
                            {
                                if (ImGui::Combo(param.name.c_str(), (int*)param.data, fractalTypes, IM_ARRAYSIZE(fractalTypes)))
                                {
                                    updated = updated || pass->GetPassType() != TerrainPassTypes::HYDROLOGY;
                                }
                                break;
                            }
                            break;
                            
                        }
                        default:
                        {
                            PWG_ERROR("Wrong control type");
                            break;
                        }
                        }
                    }
                    ImGui::PopID();
                }

                if (updated)
                {
                    chunkManager->SetNeedRegenerate(true);
                }

                ImGui::EndTabItem();
            }
        }
    };

    inline void ShowMaskPreviews(std::shared_ptr<ChunkManager> chunkManager)
    {
        for (auto& mask : chunkManager->GetMasks())
        {
            if (mask.first == "FinalHeight" || mask.first.starts_with("preview_"))
            {
                continue;
            }

            ImGui::PushID(mask.first.c_str());
            ImGui::Text("%s", mask.first.c_str());
            ImGui::Image(
                (void*)(intptr_t)mask.second->texture->GetTextureID(),
                ImVec2(200.0f, 200.0f),
                ImVec2(0, 1),
                ImVec2(1, 0));
            ImGui::PopID();
            ImGui::Separator();
        }
    }
}

#endif // !SRC_SCENE_CONTROLS_NOISE_CONTROLS_H
