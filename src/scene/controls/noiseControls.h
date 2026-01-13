#ifndef SRC_SCENE_CONTROLS_NOISE_CONTROLS_H
#define SRC_SCENE_CONTROLS_NOISE_CONTROLS_H

#include "imgui.h"
#include "FastNoiseLite.h"

namespace pwg::controls
{
	class NoiseControls
	{
	public:

		static void ShowControl(TerrainNoiseSettings& noiseSettings, int terrainSize)
		{

            if (ImGui::BeginTabItem("Noise"))
            {
                
                //General options
                static int selectedNoiseType = 0;
                int noiseType = noiseSettings.noiseType;
                float amplitude = noiseSettings.amplitude;
                float frequency = noiseSettings.frequency;
                float scale = noiseSettings.scale;
                int seed = noiseSettings.seed;
                int size = terrainSize;
                glm::vec2 offset = noiseSettings.offset;

                //Fractal
                static int selectedFractalType = 0;
                int fractalType = noiseSettings.fractalType;
                float persistance = noiseSettings.persistance;
                float lacunarity = noiseSettings.lacunarity;
                int octaves = noiseSettings.octaves;

                //DomainWarp
                static int selectedDomainWarpType = 0;
                bool domainWarp = noiseSettings.domainWarp;
                int domainWarpType = noiseSettings.domainWarpType;
                float domainWarpAmplitude = noiseSettings.domainWarpAmplitude;

                bool updated = false;

                //Types
                static const char* noiseTypes[] = { "OpenSimplex2", "OpenSimplex2S", "Cellular", "Perlin", "Value Cubic", "Value" };
                static const char* fractalTypes[] = { "None", "FBm", "Ridged" };
                static const char* domainWarpTypes[] = { "OpenSimplex2", "OpenSimplex2 Reduced", "Basic Grid" };

                //General controls
                ImGui::TextUnformatted("General");

                if (ImGui::Combo("Noise type", &selectedNoiseType, noiseTypes, IM_ARRAYSIZE(noiseTypes)))
                {
                    noiseSettings.noiseType = (FastNoiseLite::NoiseType)selectedNoiseType;
                    updated = true;
                }

                if (ImGui::SliderFloat("Amplitude", &amplitude, 0.0f, 5000.0f)) { noiseSettings.amplitude = amplitude; updated = true; }
                if (ImGui::SliderFloat("Frequency", &frequency, 0.001f, 10.0f)) { noiseSettings.frequency = frequency; updated = true; }
                if (ImGui::SliderFloat("Scale", &scale, 0.1f, 400.0f)) { noiseSettings.scale = scale; updated = true; }
                if (ImGui::DragFloat2("Offset", &offset.x, 0.1f, 100.0f)) { noiseSettings.offset.x = offset.x; noiseSettings.offset.y = offset.y; updated = true; }
                if (ImGui::InputInt("Seed", &seed)) { noiseSettings.seed = seed; updated = true; }

                ImGui::Separator();

                ImGui::TextUnformatted("Fractal");

                if (ImGui::Combo("Fractal type", &selectedFractalType, fractalTypes, IM_ARRAYSIZE(fractalTypes)))
                {
                    noiseSettings.fractalType = (FastNoiseLite::FractalType)selectedFractalType;
                    updated = true;
                }
                
                ImGui::BeginDisabled(fractalType == 0);

                if (ImGui::SliderInt("Octaves", &octaves, 1, 16)) { noiseSettings.octaves = octaves; updated = true; }
                if (ImGui::SliderFloat("Persistance", &persistance, 0.01f, 2.0f)) { noiseSettings.persistance = persistance; updated = true; }
                if (ImGui::SliderFloat("Lacunarity", &lacunarity, 0.01f, 10.0f)) { noiseSettings.lacunarity = lacunarity; updated = true; }

                ImGui::EndDisabled();

                ImGui::Separator();

                ImGui::TextUnformatted("Domain Warp");

                if (ImGui::Checkbox("Domain Warp Enabled", &domainWarp)) { noiseSettings.domainWarp = domainWarp; updated = true; }

                ImGui::BeginDisabled(domainWarp == false);

                if (ImGui::Combo("Domain Warp type", &selectedDomainWarpType, domainWarpTypes, IM_ARRAYSIZE(domainWarpTypes)))
                {
                    noiseSettings.domainWarpType = (FastNoiseLite::DomainWarpType)selectedDomainWarpType;
                    updated = true;
                }

                if (ImGui::SliderFloat("Domain Warp Amplitude", &domainWarpAmplitude, 0.0f, 100.0f)) { noiseSettings.domainWarpAmplitude = domainWarpAmplitude; updated = true; }

                ImGui::EndDisabled();

                if (updated)
                {
                    noiseSettings.dirty = true;
                }

                ImGui::EndTabItem();
            }
		}
	};
}

#endif // !SRC_SCENE_CONTROLS_NOISE_CONTROLS_H
