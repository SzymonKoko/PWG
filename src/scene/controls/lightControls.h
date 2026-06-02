#ifndef SRC_SCENE_CONTROLS_LIGHT_CONTROLS_H
#define SRC_SCENE_CONTROLS_LIGHT_CONTROLS_H

#include "imgui.h"
#include "FastNoiseLite.h"
#include "scene/objects/sunObject.h"

namespace pwg::controls
{
    class LightControls
    {
    public:

        static void ShowControl(std::shared_ptr<SunObject> sun)
        {
            bool propertiesUpdated = false;
            bool lightUpdated = false;

            auto& light = sun->GetLight();
            auto& properties = sun->GetSunObjectProperties();

            if (ImGui::Checkbox("Start/Stop time", &properties.stopTime));

            if (ImGui::SliderFloat("Time", &properties.time, 0.0f, 360.0f));

            if (ImGui::ColorEdit3("Light color", &light.color.x));

            if (ImGui::SliderFloat("Light intensity", &light.intensivity, 0.0, 20.0f));

            ImGui::Separator();

            ImGui::Text("Rendering");
            if (ImGui::SliderFloat("Exposure", &light.exposure, 0.2f, 3.0f));
            if (ImGui::SliderFloat("Sky ambient", &light.skyAmbient, 0.0f, 0.35f));
            if (ImGui::SliderFloat("Ground ambient", &light.groundAmbient, 0.0f, 0.2f));

            ImGui::Separator();

            ImGui::Text("Terrain material");
            if (ImGui::SliderFloat("Normal strength", &light.normalStrength, 0.0f, 1.5f));
            if (ImGui::SliderFloat("Snow height", &light.snowHeight, 0.0f, 5000.0f));
            if (ImGui::SliderFloat("Snow softness", &light.snowSoftness, 50.0f, 2500.0f));
            if (ImGui::SliderFloat("Wetness strength", &light.wetnessStrength, 0.0f, 1.5f));
            if (ImGui::SliderFloat("Water alpha", &light.waterAlpha, 0.05f, 1.0f));

            ImGui::Separator();
            
            if (ImGui::SliderFloat("Sun speed", &properties.orbitSpeed, 0.001f, 1.0f));

        }
    };
}

#endif // !SRC_SCENE_CONTROLS_LIGHT_CONTROLS_H
