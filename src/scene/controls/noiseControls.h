#ifndef SRC_SCENE_CONTROLS_NOISE_CONTROLS_H
#define SRC_SCENE_CONTROLS_NOISE_CONTROLS_H

#include "procedural/noiseTexture.h"
#include "imgui.h"

namespace pwg::controls
{
	class NoiseControls
	{
	public:

		static void ShowControl(NoiseTexture& noiseTexture)
		{
            if (ImGui::BeginTabItem("Noise"))
            {
                float amplitude = noiseTexture.GetNoiseParameters().amplitude;
                float frequency = noiseTexture.GetNoiseParameters().frequency;
                float scale = noiseTexture.GetNoiseParameters().scale;
                glm::vec2 offset = noiseTexture.GetNoiseParameters().offset;
                int seed = noiseTexture.GetNoiseParameters().seed;
                int octaves = noiseTexture.GetNoiseParameters().octaves;
                float persistance = noiseTexture.GetNoiseParameters().persistance;
                float lacunarity = noiseTexture.GetNoiseParameters().lacunarity;

                bool updated = false;

                const char* noiseTypes[] = { "Perlin", "Simplex", "Cellular", "Value" };
                static int selectedType = 1;

                if (ImGui::Combo("Noise type", &selectedType, noiseTypes, IM_ARRAYSIZE(noiseTypes)))
                {
                    if (selectedType == 0)
                    {
                        noiseTexture.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
                    }
                    else if (selectedType == 1)
                    {
                        noiseTexture.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
                    }
                    else if (selectedType == 2)
                    {
                        noiseTexture.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
                    }
                    else if (selectedType == 3)
                    {
                        noiseTexture.SetNoiseType(FastNoiseLite::NoiseType_Value);
                    }
                }

                if (ImGui::SliderFloat("Amplitude", &amplitude, 0.0f, 10.0f)) { noiseTexture.SetAmplitude(amplitude); updated = true; }
                if (ImGui::SliderFloat("Frequency", &frequency, 0.001f, 1.0f)) { noiseTexture.SetFrequency(frequency); updated = true; }
                if (ImGui::SliderFloat("Scale", &scale, 0.1f, 200.0f)) { noiseTexture.SetScale(scale); updated = true; }
                if (ImGui::SliderInt("Octaves", &octaves, 1, 8)) { noiseTexture.SetOctaves(octaves); updated = true; }
                if (ImGui::SliderFloat("Persistance", &persistance, 0.01f, 2.0f)) { noiseTexture.SetPersistance(persistance); updated = true; }
                if (ImGui::SliderFloat("Lacunarity", &lacunarity, 0.01f, 10.0f)) { noiseTexture.SetLacunarity(lacunarity); updated = true; }
                if (ImGui::DragFloat2("Offset", &offset.x, 0.1f, 100.0f)) { noiseTexture.SetOffset(offset); updated = true; }
                if (ImGui::InputInt("Seed", &seed)) { noiseTexture.SetSeed(seed); updated = true; }

                if (updated)
                {
                    noiseTexture.UpdateNoiseData(noiseTexture.GetNoiseParameters());
                }

                ImVec2 windowSize = ImGui::GetContentRegionAvail();

                if (noiseTexture.GetTextureID() != 0)
                {
                    ImVec2 windowSize = ImGui::GetContentRegionAvail();
                    ImVec2 noiseSize((float)noiseTexture.GetNoiseWidth(), (float)noiseTexture.GetNoiseHeight());

                    //Center noise image on the window
                    ImVec2 cursorPos = ImGui::GetCursorPos();
                    ImVec2 offset((windowSize.x - noiseSize.x) * 0.5f, (windowSize.y - noiseSize.y) * 0.5f);

                    ImGui::SetCursorPos(ImVec2(cursorPos.x + offset.x, cursorPos.y + offset.y));

                    ImGui::Image(
                        (ImTextureID)(intptr_t)noiseTexture.GetTextureID(),
                        noiseSize,
                        ImVec2(0, 1), ImVec2(1, 0)
                    );
                }
                else
                {
                    ImGui::Text("Noise texture not initialized.");
                }

                ImGui::EndTabItem();
            }
		}
	};
}

#endif // !SRC_SCENE_CONTROLS_NOISE_CONTROLS_H
