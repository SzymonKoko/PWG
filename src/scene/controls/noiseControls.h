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
            static bool runSimulation = false;
            static float simulationAccumulator = 0.0f;
            static int terrainPreset = 2;
            static bool showAdvancedErosion = false;
            bool updated = false;
            
            auto terrainComputePipeline = chunkManager->GetTerrainComputePipeline();


            if (ImGui::BeginTabItem("Noise"))
            {
                auto& passes = terrainComputePipeline->GetGraph()->GetPasses();

                if (ImGui::Button("Regenerate Terrain"))
                {
                    runSimulation = false;
                    simulationAccumulator = 0.0f;
                    chunkManager->Regenerate();
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset Water"))
                {
                    runSimulation = false;
                    simulationAccumulator = 0.0f;
                    chunkManager->ResetWaterSimulation();
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset Terrain Erosion"))
                {
                    runSimulation = false;
                    simulationAccumulator = 0.0f;
                    chunkManager->ResetErosionSimulation();
                }
                if (ImGui::Button("Step Simulation"))
                {
                    chunkManager->StepSimulation();
                }
                ImGui::Checkbox("Run Simulation", &runSimulation);
                if (ImGui::Combo("Terrain preset", &terrainPreset, terrainPresets, IM_ARRAYSIZE(terrainPresets)))
                {
                    runSimulation = false;
                    simulationAccumulator = 0.0f;
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
                ImGui::Text("Shallow Water");
                ImGui::Checkbox("Enable erosion", &hydraulicSettings.erosionEnabled);
                ImGui::Checkbox("Show water surface", &hydraulicSettings.showWaterSurface);
                ImGui::Checkbox("Auto-run simulation", &hydraulicSettings.autoWaterSimulation);

                ImGui::Separator();
                ImGui::Text("Rain and flow");
                ImGui::SliderFloat("Rain amount", &hydraulicSettings.rainRate, 0.0f, 0.015f);
                ImGui::SliderFloat("Rain mountain boost", &hydraulicSettings.rainElevationBias, 0.0f, 1.0f);
                ImGui::SliderFloat("Pipe flow speed", &hydraulicSettings.pipeFlow, 0.05f, 0.9f);
                ImGui::SliderFloat("River concentration", &hydraulicSettings.flowConcentration, 0.0f, 0.85f);
                ImGui::SliderFloat("Evaporation", &hydraulicSettings.evaporation, 0.0f, 0.002f);
                ImGui::SliderFloat("Max water depth", &hydraulicSettings.maxWaterDepth, 0.5f, 6.0f);

                ImGui::Separator();
                ImGui::Text("Erosion (terrain carving)");
                ImGui::SliderFloat("Rock erosion", &hydraulicSettings.erosionStrength, 0.0f, 0.6f);
                ImGui::SliderFloat("Terrain flattening", &hydraulicSettings.flatteningStrength, 0.0f, 1.0f);
                ImGui::SliderFloat("Sediment deposit (cosmetic)", &hydraulicSettings.depositionStrength, 0.0f, 0.08f);
                ImGui::SliderFloat("River channel depth", &hydraulicSettings.riverChannelDepth, 0.0f, 0.3f);

                ImGui::Separator();
                ImGui::Text("Lakes");
                ImGui::SliderFloat("Lake min depth", &hydraulicSettings.lakeMinDepth, 0.005f, 0.1f);
                ImGui::SliderFloat("Lake leveling", &hydraulicSettings.lakeLeveling, 0.0f, 0.8f);

                ImGui::Separator();
                ImGui::Text("Simulation speed");
                ImGui::SliderInt("Steps per click", &hydraulicSettings.stepsPerUpdate, 1, 16);
                ImGui::SliderInt("Steps per second", &hydraulicSettings.waterStepsPerSecond, 1, 60);
                ImGui::SliderInt("Substeps", &hydraulicSettings.waterSubsteps, 1, 8);

                if (ImGui::Button("Balanced preset"))
                {
                    hydraulicSettings.rainRate = 0.004f;
                    hydraulicSettings.rainElevationBias = 0.35f;
                    hydraulicSettings.pipeFlow = 0.38f;
                    hydraulicSettings.pipeDamping = 0.28f;
                    hydraulicSettings.flowConcentration = 0.40f;
                    hydraulicSettings.evaporation = 0.00004f;
                    hydraulicSettings.maxWaterDepth = 2.5f;
                    hydraulicSettings.erosionStrength = 0.20f;
                    hydraulicSettings.flatteningStrength = 0.25f;
                    hydraulicSettings.depositionStrength = 0.014f;
                    hydraulicSettings.riverChannelDepth = 0.14f;
                    hydraulicSettings.lakeMinDepth = 0.012f;
                    hydraulicSettings.lakeLeveling = 0.48f;
                }
                ImGui::SameLine();
                if (ImGui::Button("Deep rivers preset"))
                {
                    hydraulicSettings.rainRate = 0.005f;
                    hydraulicSettings.rainElevationBias = 0.25f;
                    hydraulicSettings.pipeFlow = 0.48f;
                    hydraulicSettings.pipeDamping = 0.32f;
                    hydraulicSettings.flowConcentration = 0.55f;
                    hydraulicSettings.evaporation = 0.00003f;
                    hydraulicSettings.maxWaterDepth = 3.0f;
                    hydraulicSettings.erosionStrength = 0.28f;
                    hydraulicSettings.flatteningStrength = 0.15f;
                    hydraulicSettings.depositionStrength = 0.015f;
                    hydraulicSettings.riverChannelDepth = 0.20f;
                    hydraulicSettings.lakeMinDepth = 0.010f;
                    hydraulicSettings.lakeLeveling = 0.40f;
                }
                ImGui::SameLine();
                if (ImGui::Button("Many lakes preset"))
                {
                    hydraulicSettings.rainRate = 0.006f;
                    hydraulicSettings.rainElevationBias = 0.15f;
                    hydraulicSettings.pipeFlow = 0.30f;
                    hydraulicSettings.pipeDamping = 0.22f;
                    hydraulicSettings.flowConcentration = 0.30f;
                    hydraulicSettings.evaporation = 0.00002f;
                    hydraulicSettings.maxWaterDepth = 4.0f;
                    hydraulicSettings.erosionStrength = 0.10f;
                    hydraulicSettings.flatteningStrength = 0.35f;
                    hydraulicSettings.depositionStrength = 0.018f;
                    hydraulicSettings.riverChannelDepth = 0.10f;
                    hydraulicSettings.lakeMinDepth = 0.008f;
                    hydraulicSettings.lakeLeveling = 0.65f;
                }

                ImGui::Checkbox("Show advanced erosion controls", &showAdvancedErosion);
                ImGui::Separator();

                bool hydrologyHeaderShown = false;
                bool erosionHeaderShown = false;
                for (const auto& pass : passes)
                {
                    if (pass->GetPassType() == TerrainPassTypes::EROSION && !showAdvancedErosion)
                    {
                        continue;
                    }
                    if (pass->GetPassType() == TerrainPassTypes::HYDROLOGY && !hydrologyHeaderShown)
                    {
                        ImGui::Separator();
                        ImGui::Text("Water Physics");
                        hydrologyHeaderShown = true;
                    }
                    if (pass->GetPassType() == TerrainPassTypes::EROSION && !erosionHeaderShown)
                    {
                        ImGui::Separator();
                        ImGui::Text("Hydraulic Erosion");
                        erosionHeaderShown = true;
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
                                updated = updated || (pass->GetPassType() != TerrainPassTypes::HYDROLOGY && pass->GetPassType() != TerrainPassTypes::EROSION);
                            }
                            break;
                        }
                        case ParameterType::Int:
                        {
                            if (ImGui::SliderInt(param.name.c_str(), (int*)param.data, param.min, param.max))
                            {
                                updated = updated || (pass->GetPassType() != TerrainPassTypes::HYDROLOGY && pass->GetPassType() != TerrainPassTypes::EROSION);
                            }
                            break;
                        }
                        case ParameterType::Bool:
                        {
                            if (ImGui::Checkbox(param.name.c_str(), (bool*)param.data))
                            {
                                updated = updated || (pass->GetPassType() != TerrainPassTypes::HYDROLOGY && pass->GetPassType() != TerrainPassTypes::EROSION);
                            }
                            break;
                        }
                        case ParameterType::List:
                        {
                            if (param.name == "Noise Type")
                            {
                                if (ImGui::Combo(param.name.c_str(), (int*)param.data, noiseTypes, IM_ARRAYSIZE(noiseTypes)))
                                {
                                    updated = updated || (pass->GetPassType() != TerrainPassTypes::HYDROLOGY && pass->GetPassType() != TerrainPassTypes::EROSION);
                                }
                                break;
                            }
                            else if (param.name == "Fractal Type")
                            {
                                if (ImGui::Combo(param.name.c_str(), (int*)param.data, fractalTypes, IM_ARRAYSIZE(fractalTypes)))
                                {
                                    updated = updated || (pass->GetPassType() != TerrainPassTypes::HYDROLOGY && pass->GetPassType() != TerrainPassTypes::EROSION);
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
                    runSimulation = false;
                    simulationAccumulator = 0.0f;
                    chunkManager->SetNeedRegenerate(true);
                }

                if (runSimulation)
                {
                    float stepsPerSecond = static_cast<float>(std::max(hydraulicSettings.waterStepsPerSecond, 1));
                    float stepInterval = 1.0f / stepsPerSecond;
                    int substeps = std::clamp(hydraulicSettings.waterSubsteps, 1, 8);
                    int maxStepsThisFrame = 2;
                    simulationAccumulator += ImGui::GetIO().DeltaTime;

                    while (simulationAccumulator >= stepInterval && maxStepsThisFrame > 0)
                    {
                        for (int i = 0; i < substeps; ++i)
                        {
                            chunkManager->StepSimulation();
                        }
                        simulationAccumulator -= stepInterval;
                        maxStepsThisFrame--;
                    }

                    if (maxStepsThisFrame == 0)
                    {
                        simulationAccumulator = 0.0f;
                    }

                    chunkManager->MaybeUpdateHeightCache(ImGui::GetIO().DeltaTime);
                }
                else
                {
                    simulationAccumulator = 0.0f;
                }

                ImGui::EndTabItem();
            }
        }
    };

    inline void ShowMaskPreviews(std::shared_ptr<ChunkManager> chunkManager)
    {
        for (auto& mask : chunkManager->GetMasks())
        {
            if (mask.first == "FinalHeight")
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
