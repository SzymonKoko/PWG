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
            static bool runParticles = false;
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
                    runParticles = false;
                    simulationAccumulator = 0.0f;
                    chunkManager->Regenerate();
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset Water"))
                {
                    runParticles = false;
                    simulationAccumulator = 0.0f;
                    chunkManager->ResetWaterSimulation();
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset Terrain Erosion"))
                {
                    runParticles = false;
                    simulationAccumulator = 0.0f;
                    chunkManager->ResetErosionSimulation();
                }
                if (ImGui::Button("Step Particles"))
                {
                    chunkManager->StepErosionSimulation();
                }
                ImGui::SameLine();
                if (chunkManager->IsParticleBakeActive())
                {
                    if (ImGui::Button("Cancel Bake"))
                    {
                        chunkManager->CancelParticleBake();
                    }
                    int bakeStep = chunkManager->GetParticleBakeStep();
                    int bakeTotal = std::max(chunkManager->GetParticleBakeTotal(), 1);
                    float progress = static_cast<float>(bakeStep) / static_cast<float>(bakeTotal);
                    ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
                    ImGui::Text("Particle bake: %d / %d", bakeStep, bakeTotal);
                }
                else
                {
                    if (ImGui::Button("Start Particle Bake"))
                    {
                        runParticles = false;
                        simulationAccumulator = 0.0f;
                        chunkManager->StartParticleBake();
                    }
                }
                ImGui::Checkbox("Run Particles", &runParticles);
                if (ImGui::Combo("Terrain preset", &terrainPreset, terrainPresets, IM_ARRAYSIZE(terrainPresets)))
                {
                    runParticles = false;
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
                ImGui::Text("Erosion");
                ImGui::Checkbox("Enable erosion", &hydraulicSettings.particleErosionEnabled);
                ImGui::SliderFloat("Erosion strength", &hydraulicSettings.particleErosionStrength, 0.0f, 0.16f);
                ImGui::SliderFloat("River carving", &hydraulicSettings.particleChannelStrength, 0.0f, 0.12f);
                ImGui::SliderFloat("Sediment amount", &hydraulicSettings.particleDepositionStrength, 0.0f, 0.4f);
                ImGui::SliderFloat("Sediment smoothing", &hydraulicSettings.looseSedimentSmoothing, 0.0f, 0.35f);

                ImGui::Separator();
                ImGui::Text("Water and lakes");
                bool lakeFill = hydraulicSettings.lakeFillEnabled > 0.5f;
                if (ImGui::Checkbox("Generate lakes/puddles", &lakeFill))
                {
                    hydraulicSettings.lakeFillEnabled = lakeFill ? 1.0f : 0.0f;
                }
                ImGui::Checkbox("Show water surface", &hydraulicSettings.showWaterSurface);
                ImGui::SliderFloat("Lake amount", &hydraulicSettings.lakeMaxDepth, 0.05f, 6.0f);
                ImGui::SliderFloat("Lake sensitivity", &hydraulicSettings.lakeMinFlow, 0.0f, 0.12f);

                ImGui::Separator();
                ImGui::Text("Simulation quality");
                ImGui::SliderInt("Particles per step", &hydraulicSettings.particlesPerStep, 500, 40000);
                ImGui::SliderInt("Bake steps", &hydraulicSettings.bakeParticleSteps, 1, 200);
                ImGui::SliderInt("Bake speed", &hydraulicSettings.bakeStepsPerFrame, 1, 8);
                ImGui::SliderInt("Preview every N steps", &hydraulicSettings.bakeUploadInterval, 1, 32);

                if (ImGui::Button("Balanced preset"))
                {
                    hydraulicSettings.particlesPerStep = 2500;
                    hydraulicSettings.dropletLifetime = 36;
                    hydraulicSettings.particleErosionStrength = 0.045f;
                    hydraulicSettings.particleDepositionStrength = 0.18f;
                    hydraulicSettings.particleChannelStrength = 0.04f;
                    hydraulicSettings.looseSedimentSmoothing = 0.12f;
                    hydraulicSettings.lakeMinFlow = 0.015f;
                    hydraulicSettings.lakeMaxDepth = 2.5f;
                }
                ImGui::SameLine();
                if (ImGui::Button("Deep rivers preset"))
                {
                    hydraulicSettings.particlesPerStep = 5000;
                    hydraulicSettings.dropletLifetime = 52;
                    hydraulicSettings.particleErosionStrength = 0.07f;
                    hydraulicSettings.particleDepositionStrength = 0.14f;
                    hydraulicSettings.particleChannelStrength = 0.075f;
                    hydraulicSettings.looseSedimentSmoothing = 0.08f;
                    hydraulicSettings.lakeMinFlow = 0.01f;
                    hydraulicSettings.lakeMaxDepth = 3.5f;
                }
                ImGui::SameLine();
                if (ImGui::Button("Soft sediment preset"))
                {
                    hydraulicSettings.particlesPerStep = 3000;
                    hydraulicSettings.dropletLifetime = 42;
                    hydraulicSettings.particleErosionStrength = 0.035f;
                    hydraulicSettings.particleDepositionStrength = 0.28f;
                    hydraulicSettings.particleChannelStrength = 0.035f;
                    hydraulicSettings.looseSedimentSmoothing = 0.22f;
                    hydraulicSettings.lakeMinFlow = 0.012f;
                    hydraulicSettings.lakeMaxDepth = 3.0f;
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
                    runParticles = false;
                    simulationAccumulator = 0.0f;
                    chunkManager->SetNeedRegenerate(true);
                }

                if (runParticles)
                {
                    auto& hydraulicSettings = terrainComputePipeline->GetHydraulicSettings();
                    float stepsPerSecond = static_cast<float>(std::max(hydraulicSettings.waterStepsPerSecond, 1));
                    float stepInterval = 1.0f / stepsPerSecond;
                    int substeps = std::clamp(hydraulicSettings.waterSubsteps, 1, 8);
                    int maxStepsThisFrame = 1;
                    simulationAccumulator += ImGui::GetIO().DeltaTime;

                    while (simulationAccumulator >= stepInterval && maxStepsThisFrame > 0)
                    {
                        for (int i = 0; i < substeps; ++i)
                        {
                            chunkManager->StepErosionSimulation();
                        }
                        simulationAccumulator -= stepInterval;
                        maxStepsThisFrame--;
                    }

                    if (maxStepsThisFrame == 0)
                    {
                        simulationAccumulator = 0.0f;
                    }
                }
                else
                {
                    simulationAccumulator = 0.0f;
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Masks"))
            {
                auto& masks = terrainComputePipeline->GetMasks();

                for (auto& mask : chunkManager->GetMasks())
                {
                    if (mask.first == "FinalHeight")
                    {
                        continue;
                    }

                    ImGui::PushID(mask.first.c_str());
                    ImGui::Text(mask.first.c_str());

                    ImGui::Image(
                        (void*)(intptr_t)mask.second->texture->GetTextureID(),
                        ImVec2(200.0f, 200.0f),
                        ImVec2(0, 1),
                        ImVec2(1, 0)
                    );

                    ImGui::PopID();
                    ImGui::Separator();
                }
                ImGui::EndTabItem();
            }

        }
    };
}

#endif // !SRC_SCENE_CONTROLS_NOISE_CONTROLS_H
