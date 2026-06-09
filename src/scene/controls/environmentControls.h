#ifndef SRC_SCENE_CONTROLS_ENVIRONMENT_CONTROLS_H_
#define SRC_SCENE_CONTROLS_ENVIRONMENT_CONTROLS_H_

#include "imgui.h"
#include "scene/environment/environmentSystem.h"

#include <memory>

namespace pwg::controls
{
	class EnvironmentControls
	{
	public:
		static bool ShowControl(std::shared_ptr<EnvironmentSystem> environment)
		{
			if (!environment)
			{
				return false;
			}

			bool changed = false;
			auto& settings = environment->GetSettings();

			changed |= ImGui::Checkbox("Animate time", &settings.animateTime);
			changed |= ImGui::SliderFloat("Time of day", &settings.timeOfDay, 0.0f, 24.0f, "%.2f h");
			changed |= ImGui::SliderFloat("Time scale", &settings.timeScale, 0.001f, 2.0f, "%.3f");
			changed |= ImGui::SliderFloat("Latitude", &settings.latitude, -80.0f, 80.0f, "%.1f");

			ImGui::Separator();
			ImGui::Text("Sun");
			changed |= ImGui::ColorEdit3("Sun color", &settings.sunColor.x);
			changed |= ImGui::SliderFloat("Sun illuminance", &settings.sunIlluminance, 0.0f, 20.0f, "%.2f");
			changed |= ImGui::SliderFloat("Sun angular size", &settings.sunAngularSize, 0.1f, 2.0f, "%.2f deg");

			ImGui::Separator();
			ImGui::Text("Atmosphere");
			changed |= ImGui::ColorEdit3("Sky tint", &settings.skyTint.x);
			changed |= ImGui::ColorEdit3("Ground albedo", &settings.groundAlbedo.x);
			changed |= ImGui::SliderFloat("Atmosphere density", &settings.atmosphereDensity, 0.1f, 4.0f, "%.2f");
			changed |= ImGui::SliderFloat("Haze", &settings.haze, 0.0f, 2.0f, "%.2f");
			changed |= ImGui::SliderFloat("Fog density", &settings.fogDensity, 0.0f, 0.0005f, "%.6f");
			changed |= ImGui::SliderFloat("Sky ambient", &settings.skyAmbient, 0.0f, 0.5f, "%.3f");
			changed |= ImGui::SliderFloat("Ground ambient", &settings.groundAmbient, 0.0f, 0.25f, "%.3f");
			changed |= ImGui::Checkbox("Blend cubemap skybox", &settings.useSkyboxBlend);
			if (settings.useSkyboxBlend)
			{
				changed |= ImGui::SliderFloat("Cubemap blend", &settings.skyboxBlend, 0.0f, 1.0f, "%.2f");
			}

			ImGui::Separator();
			ImGui::Text("Postprocess");
			changed |= ImGui::SliderFloat("Exposure", &settings.exposure, 0.1f, 4.0f, "%.2f");
			changed |= ImGui::SliderFloat("White point", &settings.whitePoint, 1.0f, 16.0f, "%.2f");
			changed |= ImGui::Checkbox("Bloom", &settings.bloomEnabled);
			changed |= ImGui::SliderFloat("Bloom strength", &settings.bloomStrength, 0.0f, 0.5f, "%.3f");
			changed |= ImGui::Checkbox("FXAA", &settings.fxaaEnabled);

			ImGui::Separator();
			ImGui::Text("Sun shadows");
			ImGui::Text("Mode: Cascaded shadow maps (3)");
			changed |= ImGui::Checkbox("Enable shadows", &settings.shadowsEnabled);
			changed |= ImGui::SliderInt("Shadow resolution", &settings.shadowResolution, 512, 4096);
			changed |= ImGui::SliderFloat("Shadow distance", &settings.shadowDistance, 250.0f, 8000.0f, "%.0f");
			changed |= ImGui::SliderFloat("Shadow strength", &settings.shadowStrength, 0.0f, 1.0f, "%.2f");
			changed |= ImGui::SliderFloat("Shadow bias", &settings.shadowBias, 0.0001f, 0.02f, "%.5f");
			changed |= ImGui::SliderFloat("Shadow normal bias", &settings.shadowNormalBias, 0.0f, 0.05f, "%.4f");

			ImGui::Separator();
			ImGui::Text("Terrain material");
			changed |= ImGui::SliderFloat("Normal strength", &settings.normalStrength, 0.0f, 1.5f, "%.2f");
			changed |= ImGui::SliderFloat("Snow height", &settings.snowHeight, 0.0f, 5000.0f, "%.0f");
			changed |= ImGui::SliderFloat("Snow softness", &settings.snowSoftness, 50.0f, 2500.0f, "%.0f");
			changed |= ImGui::SliderFloat("Wetness strength", &settings.wetnessStrength, 0.0f, 1.5f, "%.2f");
			changed |= ImGui::SliderFloat("Water alpha", &settings.waterAlpha, 0.05f, 1.0f, "%.2f");

			return changed;
		}
	};
}

#endif // SRC_SCENE_CONTROLS_ENVIRONMENT_CONTROLS_H_
