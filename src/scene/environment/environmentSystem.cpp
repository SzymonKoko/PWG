#include "environmentSystem.h"

#include <algorithm>
#include <cmath>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

namespace pwg
{
	namespace
	{
		constexpr float Pi = 3.14159265359f;

		glm::vec3 SunColorFromElevation(float elevation)
		{
			float horizon = 1.0f - std::clamp(elevation / 0.28f, 0.0f, 1.0f);
			glm::vec3 noonColor{ 1.0f, 0.985f, 0.94f };
			glm::vec3 lowColor{ 1.0f, 0.62f, 0.34f };
			glm::vec3 horizonColor{ 1.0f, 0.34f, 0.12f };
			return glm::mix(noonColor, glm::mix(lowColor, horizonColor, horizon), horizon);
		}

		float DaylightFromElevation(float elevation)
		{
			float civilTwilight = glm::smoothstep(-0.08f, 0.02f, elevation);
			float sunAboveHorizon = glm::smoothstep(-0.02f, 0.16f, elevation);
			return civilTwilight * sunAboveHorizon;
		}
	}

	EnvironmentSystem::EnvironmentSystem()
	{
		RecalculateSun();
	}

	void EnvironmentSystem::Update(float dt)
	{
		if (m_settings.animateTime)
		{
			m_settings.timeOfDay += dt * m_settings.timeScale;
			while (m_settings.timeOfDay >= 24.0f)
			{
				m_settings.timeOfDay -= 24.0f;
			}
		}
		RecalculateSun();
	}

	void EnvironmentSystem::ResetToDefaults()
	{
		m_settings = EnvironmentSettings{};
		RecalculateSun();
	}

	EnvironmentSettings& EnvironmentSystem::GetSettings()
	{
		return m_settings;
	}

	const EnvironmentSettings& EnvironmentSystem::GetSettings() const
	{
		return m_settings;
	}

	EnvironmentLighting EnvironmentSystem::GetLighting() const
	{
		glm::vec3 toSun = -m_settings.sunDirection;
		float elevationStrength = std::clamp(toSun.y, 0.0f, 1.0f);
		float daylight = DaylightFromElevation(toSun.y);

		EnvironmentLighting lighting;
		lighting.sunDirection = m_settings.sunDirection;
		lighting.sunColor = m_settings.sunColor * SunColorFromElevation(elevationStrength);
		lighting.sunIntensity = m_settings.sunIlluminance * daylight;
		lighting.sunAngularSize = m_settings.sunAngularSize;
		lighting.skyTint = m_settings.skyTint;
		lighting.groundAlbedo = m_settings.groundAlbedo;
		lighting.atmosphereDensity = m_settings.atmosphereDensity;
		lighting.haze = m_settings.haze;
		lighting.fogDensity = m_settings.fogDensity;
		lighting.useSkyboxBlend = m_settings.useSkyboxBlend;
		lighting.skyboxBlend = m_settings.skyboxBlend;
		lighting.exposure = m_settings.exposure;
		lighting.skyAmbient = m_settings.skyAmbient;
		lighting.groundAmbient = m_settings.groundAmbient;
		lighting.shadowsEnabled = m_settings.shadowsEnabled;
		lighting.shadowResolution = m_settings.shadowResolution;
		lighting.shadowDistance = m_settings.shadowDistance;
		lighting.shadowStrength = m_settings.shadowStrength;
		lighting.shadowBias = m_settings.shadowBias;
		lighting.shadowNormalBias = m_settings.shadowNormalBias;
		lighting.normalStrength = m_settings.normalStrength;
		lighting.snowHeight = m_settings.snowHeight;
		lighting.snowSoftness = m_settings.snowSoftness;
		lighting.wetnessStrength = m_settings.wetnessStrength;
		lighting.waterAlpha = m_settings.waterAlpha;
		return lighting;
	}

	void EnvironmentSystem::RecalculateSun()
	{
		float hourAngle = ((m_settings.timeOfDay - 12.0f) / 24.0f) * 2.0f * Pi;
		float latitude = glm::radians(std::clamp(m_settings.latitude, -80.0f, 80.0f));
		float seasonalDeclination = glm::radians(12.0f);

		float elevation = std::asin(
			std::sin(latitude) * std::sin(seasonalDeclination) +
			std::cos(latitude) * std::cos(seasonalDeclination) * std::cos(hourAngle)
		);
		float azimuth = std::atan2(
			-std::sin(hourAngle),
			std::tan(seasonalDeclination) * std::cos(latitude) - std::sin(latitude) * std::cos(hourAngle)
		);

		glm::vec3 toSun{
			std::cos(elevation) * std::sin(azimuth),
			std::sin(elevation),
			std::cos(elevation) * std::cos(azimuth)
		};
		toSun = glm::normalize(toSun);
		m_settings.sunDirection = -toSun;

	}
}
