#ifndef SRC_SCENE_ENVIRONMENT_ENVIRONMENT_SETTINGS_H_
#define SRC_SCENE_ENVIRONMENT_ENVIRONMENT_SETTINGS_H_

#include <glm/vec3.hpp>

namespace pwg
{
	struct EnvironmentSettings
	{
		float timeOfDay{ 14.0f };
		float latitude{ 45.0f };
		bool animateTime{ false };
		float timeScale{ 0.05f };

		glm::vec3 sunDirection{ -0.45f, -0.75f, -0.25f };
		float sunAngularSize{ 0.53f };
		glm::vec3 sunColor{ 1.0f, 0.96f, 0.88f };
		float sunIlluminance{ 7.5f };

		glm::vec3 skyTint{ 0.58f, 0.68f, 0.82f };
		float atmosphereDensity{ 1.0f };
		float haze{ 0.35f };
		glm::vec3 groundAlbedo{ 0.30f, 0.27f, 0.22f };
		bool useSkyboxBlend{ false };
		float skyboxBlend{ 0.35f };

		float exposure{ 1.0f };
		float whitePoint{ 11.2f };
		float bloomStrength{ 0.08f };
		float fogDensity{ 0.00006f };
		bool bloomEnabled{ true };
		bool fxaaEnabled{ true };

		bool shadowsEnabled{ true };
		int shadowResolution{ 2048 };
		float shadowDistance{ 2500.0f };
		float shadowStrength{ 0.55f };
		float shadowBias{ 0.0015f };
		float shadowNormalBias{ 0.006f };

		float skyAmbient{ 0.09f };
		float groundAmbient{ 0.025f };
		float normalStrength{ 0.65f };
		float snowHeight{ 1800.0f };
		float snowSoftness{ 850.0f };
		float wetnessStrength{ 0.65f };
		float waterAlpha{ 0.72f };
		glm::vec3 waterShallowColor{ 0.08f, 0.38f, 0.42f };
		glm::vec3 waterDeepColor{ 0.012f, 0.055f, 0.09f };
		float waterNormalStrength{ 0.52f };
		float waterWaveScale{ 1.0f };
		float waterWaveSpeed{ 1.0f };
		float waterRefractionStrength{ 7.0f };
		float waterAbsorption{ 0.22f };
		float waterFoamWidth{ 1.1f };
		float waterFoamIntensity{ 0.32f };
		float waterRoughness{ 0.16f };
	};

	struct EnvironmentLighting
	{
		glm::vec3 sunDirection{ -0.45f, -0.75f, -0.25f };
		glm::vec3 sunColor{ 1.0f, 0.96f, 0.88f };
		float sunIntensity{ 7.5f };
		float sunAngularSize{ 0.53f };

		glm::vec3 skyTint{ 0.58f, 0.68f, 0.82f };
		glm::vec3 groundAlbedo{ 0.30f, 0.27f, 0.22f };
		float atmosphereDensity{ 1.0f };
		float haze{ 0.35f };
		float fogDensity{ 0.00006f };
		bool useSkyboxBlend{ false };
		float skyboxBlend{ 0.35f };

		float exposure{ 1.0f };
		float skyAmbient{ 0.09f };
		float groundAmbient{ 0.025f };
		bool shadowsEnabled{ true };
		int shadowResolution{ 2048 };
		float shadowDistance{ 2500.0f };
		float shadowStrength{ 0.55f };
		float shadowBias{ 0.0015f };
		float shadowNormalBias{ 0.006f };
		float normalStrength{ 0.65f };
		float snowHeight{ 1800.0f };
		float snowSoftness{ 850.0f };
		float wetnessStrength{ 0.65f };
		float waterAlpha{ 0.72f };
		glm::vec3 waterShallowColor{ 0.08f, 0.38f, 0.42f };
		glm::vec3 waterDeepColor{ 0.012f, 0.055f, 0.09f };
		float waterNormalStrength{ 0.52f };
		float waterWaveScale{ 1.0f };
		float waterWaveSpeed{ 1.0f };
		float waterRefractionStrength{ 7.0f };
		float waterAbsorption{ 0.22f };
		float waterFoamWidth{ 1.1f };
		float waterFoamIntensity{ 0.32f };
		float waterRoughness{ 0.16f };
	};
}

#endif // SRC_SCENE_ENVIRONMENT_ENVIRONMENT_SETTINGS_H_
