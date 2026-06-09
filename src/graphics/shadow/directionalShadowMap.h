#ifndef SRC_GRAPHICS_SHADOW_DIRECTIONAL_SHADOW_MAP_H_
#define SRC_GRAPHICS_SHADOW_DIRECTIONAL_SHADOW_MAP_H_

#include "scene/environment/environmentSettings.h"
#include "scene/cameras/icamera.h"

#include <glm/mat4x4.hpp>
#include <array>
#include <memory>

namespace pwg
{
	class DirectionalShadowMap
	{
	public:
		DirectionalShadowMap();
		~DirectionalShadowMap();

		void Resize(int resolution);
		void BeginCascade(int cascadeIndex, const EnvironmentLighting& lighting, std::shared_ptr<ICamera> camera);
		void End(int viewportWidth, int viewportHeight);

		static constexpr int CascadeCount = 3;
		unsigned int GetDepthTextureID(int cascadeIndex) const;
		const glm::mat4& GetLightSpaceMatrix(int cascadeIndex) const;
		float GetCascadeSplit(int cascadeIndex) const;

	private:
		void Initialize();
		void Cleanup();
		glm::mat4 BuildLightSpaceMatrix(const EnvironmentLighting& lighting, std::shared_ptr<ICamera> camera, int cascadeIndex) const;

		std::array<unsigned int, CascadeCount> m_framebuffers{};
		std::array<unsigned int, CascadeCount> m_depthTextures{};
		std::array<glm::mat4, CascadeCount> m_lightSpaceMatrices{};
		std::array<float, CascadeCount> m_cascadeSplits{ 0.18f, 0.45f, 1.0f };
		int m_resolution{ 2048 };
	};
}

#endif // SRC_GRAPHICS_SHADOW_DIRECTIONAL_SHADOW_MAP_H_
