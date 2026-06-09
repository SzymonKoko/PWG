#include <glad/glad.h>
#include "directionalShadowMap.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/geometric.hpp>

namespace pwg
{
	DirectionalShadowMap::DirectionalShadowMap()
	{
		Initialize();
	}

	DirectionalShadowMap::~DirectionalShadowMap()
	{
		Cleanup();
	}

	void DirectionalShadowMap::Resize(int resolution)
	{
		resolution = std::clamp(resolution, 512, 8192);
		if (resolution == m_resolution && m_framebuffers[0] != 0 && m_depthTextures[0] != 0)
		{
			return;
		}

		m_resolution = resolution;
		Cleanup();
		Initialize();
	}

	void DirectionalShadowMap::BeginCascade(int cascadeIndex, const EnvironmentLighting& lighting, std::shared_ptr<ICamera> camera)
	{
		cascadeIndex = std::clamp(cascadeIndex, 0, CascadeCount - 1);
		m_lightSpaceMatrices[cascadeIndex] = BuildLightSpaceMatrix(lighting, camera, cascadeIndex);
		glViewport(0, 0, m_resolution, m_resolution);
		glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[cascadeIndex]);
		glClear(GL_DEPTH_BUFFER_BIT);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	}

	void DirectionalShadowMap::End(int viewportWidth, int viewportHeight)
	{
		glCullFace(GL_BACK);
		glDisable(GL_CULL_FACE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, viewportWidth, viewportHeight);
	}

	unsigned int DirectionalShadowMap::GetDepthTextureID(int cascadeIndex) const
	{
		cascadeIndex = std::clamp(cascadeIndex, 0, CascadeCount - 1);
		return m_depthTextures[cascadeIndex];
	}

	const glm::mat4& DirectionalShadowMap::GetLightSpaceMatrix(int cascadeIndex) const
	{
		cascadeIndex = std::clamp(cascadeIndex, 0, CascadeCount - 1);
		return m_lightSpaceMatrices[cascadeIndex];
	}

	float DirectionalShadowMap::GetCascadeSplit(int cascadeIndex) const
	{
		cascadeIndex = std::clamp(cascadeIndex, 0, CascadeCount - 1);
		return m_cascadeSplits[cascadeIndex];
	}

	void DirectionalShadowMap::Initialize()
	{
		for (int i = 0; i < CascadeCount; ++i)
		{
			glGenFramebuffers(1, &m_framebuffers[i]);
			glGenTextures(1, &m_depthTextures[i]);
			glBindTexture(GL_TEXTURE_2D, m_depthTextures[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, m_resolution, m_resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

			glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTextures[i], 0);
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
			m_lightSpaceMatrices[i] = glm::mat4(1.0f);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void DirectionalShadowMap::Cleanup()
	{
		for (int i = 0; i < CascadeCount; ++i)
		{
			if (m_depthTextures[i] != 0)
			{
				glDeleteTextures(1, &m_depthTextures[i]);
				m_depthTextures[i] = 0;
			}
			if (m_framebuffers[i] != 0)
			{
				glDeleteFramebuffers(1, &m_framebuffers[i]);
				m_framebuffers[i] = 0;
			}
		}
	}

	glm::mat4 DirectionalShadowMap::BuildLightSpaceMatrix(const EnvironmentLighting& lighting, std::shared_ptr<ICamera> camera, int cascadeIndex) const
	{
		float shadowDistance = std::max(lighting.shadowDistance, 100.0f);
		float split = m_cascadeSplits[std::clamp(cascadeIndex, 0, CascadeCount - 1)];
		float previousSplit = cascadeIndex > 0 ? m_cascadeSplits[cascadeIndex - 1] : 0.0f;
		float cascadeDistance = shadowDistance * split;
		float previousDistance = shadowDistance * previousSplit;
		glm::vec3 center = camera ? camera->GetCameraPosition() : glm::vec3(0.0f);
		center.y = 900.0f;

		glm::vec3 lightDirection = glm::normalize(lighting.sunDirection);
		glm::vec3 lightPosition = center - lightDirection * (cascadeDistance * 2.0f + 1200.0f);
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		if (std::abs(glm::dot(up, lightDirection)) > 0.92f)
		{
			up = glm::vec3(0.0f, 0.0f, 1.0f);
		}

		glm::mat4 lightView = glm::lookAt(lightPosition, center, up);
		float halfExtent = std::max(cascadeDistance - previousDistance * 0.35f, 120.0f);
		float nearPlane = 0.1f;
		float farPlane = cascadeDistance * 4.0f + 4000.0f;
		glm::mat4 lightProjection = glm::ortho(-halfExtent, halfExtent, -halfExtent, halfExtent, nearPlane, farPlane);
		return lightProjection * lightView;
	}
}
