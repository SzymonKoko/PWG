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
            
            if (ImGui::SliderFloat("Sun speed", &properties.orbitSpeed, 0.001f, 1.0f));

        }
    };
}

#endif // !SRC_SCENE_CONTROLS_LIGHT_CONTROLS_H
