#ifndef SRC_SCENE_CONTROLS_NOISE_CONTROLS_H
#define SRC_SCENE_CONTROLS_NOISE_CONTROLS_H

#include "procedural/noiseTexture.h"
#include <thread>
#include "imgui.h"

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
                int size = terrainSize;
                int noiseType = noiseSettings.noiseType;
                int seed = noiseSettings.seed;
                int octaves = noiseSettings.octaves;
                float amplitude = noiseSettings.amplitude;
                float frequency = noiseSettings.frequency;
                float scale = noiseSettings.scale;
                float persistance = noiseSettings.persistance;
                float lacunarity = noiseSettings.lacunarity;
                glm::vec2 offset = noiseSettings.offset;

                //PWG_ERROR("AMP: {0}, FREQ: {1}, TYPE: {2}", amplitude, frequency, noiseType);

                ////Fractal options
                //static int fractalType = 0;
                //static int fractalOctaves = 3;
                //static float fractalLacunarity = 2.0f;
                //static float fractalGain = 0.5f;
                //static float fractalWeightedStrength = 0.5f;
                //static float fractalPingPongStrength = 2.0f;


                //static int selectedCellularDistanceFuncType = 0;
                //static int selectedCellularReturnType = 0;
                //static float cellularJitter = 1.0f;
                bool updated = false;

                //Types
                static const char* noiseTypes[] = { "OpenSimplex2", "OpenSimplex2S", "Cellular", "Perlin", "Value Cubic", "Value" };
              /*  static const char* fractalTypes[] = { "None", "FBm", "Ridged", "Ping Pong" };
                static const char* cellularDistanceFuncTypes[] = { "Euclidean", "Euclidean Sq", "Manhattan", "Hybrid" };
                static const char* cellularReturnTypes[] = { "Cell Value", "Distance", "Distance 2", "Distance 2 Add", "Distance 2 Sub", "Distance 2 Mul", "Distance 2 Div" };*/


                

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
                if (ImGui::SliderInt("Octaves", &octaves, 1, 16)) { noiseSettings.octaves = octaves; updated = true; }
                if (ImGui::SliderFloat("Persistance", &persistance, 0.01f, 2.0f)) { noiseSettings.persistance = persistance; updated = true; }
                if (ImGui::SliderFloat("Lacunarity", &lacunarity, 0.01f, 10.0f)) { noiseSettings.lacunarity = lacunarity; updated = true; }
                if (ImGui::DragFloat2("Offset", &offset.x, 0.1f, 100.0f)) { noiseSettings.offset.x = offset.x; noiseSettings.offset.y = offset.y; updated = true; }
                if (ImGui::InputInt("Seed", &seed)) { noiseSettings.seed = seed; updated = true; }


                //if (ImGui::Combo("Fractal type", &fractalType, fractalTypes, IM_ARRAYSIZE(fractalTypes)))
                //{
                //    noiseTexture.SetFractalType((FastNoiseLite::FractalType)fractalType);
                //    updated = true;
                //}

                ////Fractal controls
                //ImGui::TextUnformatted("Fractal");

                //ImGui::BeginDisabled(fractalType == 0);
                //if (ImGui::DragInt("Fractal Octaves", &fractalOctaves, 0.1f, 1, 20))
                //{
                //    noiseTexture.SetFractalOctaves(fractalOctaves);
                //    updated = true;
                //}
                //if (ImGui::DragFloat("Fractal Lacunarity", &fractalLacunarity, 0.01f))
                //{
                //    noiseTexture.SetFractalLacunarity(fractalLacunarity);
                //    updated = true;
                //}
                //if (ImGui::DragFloat("Fractal Gain", &fractalGain, 0.01f))
                //{
                //    noiseTexture.SetFractalGain(fractalGain);
                //    updated = true;
                //}
                //if (ImGui::DragFloat("Weighted Strength", &fractalWeightedStrength, 0.01f))
                //{
                //    noiseTexture.SetFractalWeightedStrength(fractalWeightedStrength);
                //    updated = true;
                //}
                //ImGui::BeginDisabled(fractalType != (int)FastNoiseLite::FractalType_PingPong);
                //if (ImGui::DragFloat("Ping Pong Strength", &fractalPingPongStrength, 0.01f))
                //{
                //    noiseTexture.SetFractalPingPongStrength(fractalPingPongStrength);
                //    updated = true;
                //}
                //ImGui::EndDisabled();
                //ImGui::EndDisabled();

                ////Cellular controls

                //ImGui::TextUnformatted("Cellular");
                //ImGui::BeginDisabled(selectedNoiseType != (int)FastNoiseLite::NoiseType_Cellular);

                //if (ImGui::Combo("Distance Function", &selectedCellularDistanceFuncType, cellularDistanceFuncTypes, IM_ARRAYSIZE(cellularDistanceFuncTypes)))
                //{
                //    noiseTexture.SetCellularDistanceFunction((FastNoiseLite::CellularDistanceFunction)selectedCellularDistanceFuncType);
                //}
                //if (ImGui::Combo("Return Type", &selectedCellularReturnType, cellularReturnTypes, IM_ARRAYSIZE(cellularReturnTypes)))
                //{
                //    noiseTexture.SetCellularReturnType((FastNoiseLite::CellularReturnType)selectedCellularReturnType);
                //}
                //if (ImGui::DragFloat("Jitter", &cellularJitter, 0.01f))
                //{
                //    noiseTexture.SetCellularJitter(cellularJitter);
                //}

                //ImGui::EndDisabled();

                if (updated)
                {
                    noiseSettings.dirty = true;
                }

                ImVec2 windowSize = ImGui::GetContentRegionAvail();

                //if (tex != 0)
                //{

                //    ImVec2 windowSize = ImGui::GetContentRegionAvail();
                //    ImVec2 noiseSize((float)tex, (float)size);
                //    float scaleX= windowSize.x / noiseSize.x;
                //    float scaleY = windowSize.y / noiseSize.y;

                //    ImVec2 textureSize(noiseSize.x * scaleX, noiseSize.y * scaleX);

                //    textureSize.x = std::clamp(textureSize.x, 10.0f, 400.0f);
                //    textureSize.y = std::clamp(textureSize.y, 10.0f, 400.0f);

                //    //Center noise image on the window
                //    ImVec2 cursorPos = ImGui::GetCursorPos();
                //    ImVec2 offset((windowSize.x - textureSize.x) * 0.5f, (windowSize.y - textureSize.y) * 0.5f);

                //    ImGui::SetCursorPos(ImVec2(cursorPos.x + offset.x, cursorPos.y + offset.y));

                //    ImGui::Image(
                //        (ImTextureID)(intptr_t)tex,
                //        textureSize,
                //        ImVec2(0, 1), ImVec2(1, 0)
                //    );
                //}
                //else
                //{
                //    ImGui::Text("Noise texture not initialized.");
                //}

                ImGui::EndTabItem();
            }
		}
	};
}

#endif // !SRC_SCENE_CONTROLS_NOISE_CONTROLS_H
