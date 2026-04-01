#ifndef SRC_SCENE_CONTROLS_NOISE_CONTROLS_H
#define SRC_SCENE_CONTROLS_NOISE_CONTROLS_H

#include "imgui.h"
#include "FastNoiseLite.h"

namespace pwg::controls
{
	class NoiseControls
	{
	public:

		static void ShowControl(std::shared_ptr<ChunkManager> chunkManager)
		{

            static const char* noiseTypes[] = { "OpenSimplex2", "OpenSimplex2S", "Cellular", "Perlin", "Value Cubic", "Value" };
            static const char* fractalTypes[] = { "None", "FBm", "Ridged", "Ping Pong" };
            bool updated = false;
            
            auto terrainComputePipeline = chunkManager->GetTerrainComputePipeline();


            if (ImGui::BeginTabItem("Noise"))
            {
                auto& passes = terrainComputePipeline->GetGraph()->GetPasses();

                for (const auto& pass : passes)
                {
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
                                updated = true;
                            }
                            break;
                        }
                        case ParameterType::Int:
                        {
                            if (ImGui::SliderInt(param.name.c_str(), (int*)param.data, param.min, param.max))
                            {
                                updated = true;
                            }
                            break;
                        }
                        case ParameterType::Bool:
                        {
                            if (ImGui::Checkbox(param.name.c_str(), (bool*)param.data))
                            {
                                updated = true;
                            }
                            break;
                        }
                        case ParameterType::List:
                        {
                            if (param.name == "Noise Type")
                            {
                                if (ImGui::Combo(param.name.c_str(), (int*)param.data, noiseTypes, IM_ARRAYSIZE(noiseTypes)))
                                {
                                    updated = true;
                                }
                                break;
                            }
                            else if (param.name == "Fractal Type")
                            {
                                if (ImGui::Combo(param.name.c_str(), (int*)param.data, fractalTypes, IM_ARRAYSIZE(fractalTypes)))
                                {
                                    updated = true;
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

            if (ImGui::BeginTabItem("Masks"))
            {
                auto& masks = terrainComputePipeline->GetMasks();

                for (auto& mask : chunkManager->GetMasks())
                {
                    if (mask.first == "FinalHeight")
                    {
                        break;
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
