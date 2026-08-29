#include <glad/glad.h>
#include "renderer.h"
#include "core/logger/logger.h"
#include "graphics/renderer/irenderable.h"

#include <algorithm>

pwg::Renderer::Renderer()
{
	m_lightingUploader = std::make_unique<LightingUploader>();
	PWG_INFO("Renderer initialized");
}

pwg::Renderer::~Renderer()
{

}

void pwg::Renderer::BeginFrame()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glDisable(GL_FRAMEBUFFER_SRGB);

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_renderQueue.clear();
}

void pwg::Renderer::EndFrame()
{
	glDisable(GL_FRAMEBUFFER_SRGB);
}

void pwg::Renderer::Draw(IRenderable* renderable)
{
	renderable->Draw(*this);
}
void pwg::Renderer::DrawMesh(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, const glm::mat4& model)
{
	if (!mesh || !material)
	{
		return;
	}

	m_lightingUploader->Upload(material, m_environmentLighting, m_cameraPosition);
	material->SetUniformMat4("u_model", model);
	material->SetUniformMat4("u_view", m_viewMatrix);
	material->SetUniformMat4("u_projection", m_projectionMatrix);
	material->SetUniformInt("u_instanced", 0);
	material->Apply();
	mesh->Draw();
}

void pwg::Renderer::DrawMeshInstanced(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, const std::vector<glm::mat4>& transforms)
{
	if (!mesh || !material || transforms.empty())
	{
		return;
	}

	m_lightingUploader->Upload(material, m_environmentLighting, m_cameraPosition);
	material->SetUniformMat4("u_view", m_viewMatrix);
	material->SetUniformMat4("u_projection", m_projectionMatrix);
	material->SetUniformInt("u_instanced", 1);
	material->Apply();
	mesh->DrawInstanced(transforms);
}

void pwg::Renderer::PrepareMaterial(const std::shared_ptr<Material>& material)
{
	if (!material) return;
	m_lightingUploader->Upload(material, m_environmentLighting, m_cameraPosition);
	material->SetUniformMat4("u_view", m_viewMatrix);
	material->SetUniformMat4("u_projection", m_projectionMatrix);
	material->SetUniformMat4("u_inverseProjection", glm::inverse(m_projectionMatrix));
}

bool pwg::Renderer::IsSphereVisible(const glm::vec3& center, float radius, float maxDistance, float frustumMargin) const
{
	if (maxDistance > 0.0f && glm::distance(glm::vec2(center.x, center.z), glm::vec2(m_cameraPosition.x, m_cameraPosition.z)) > maxDistance + radius)
	{
		return false;
	}

	float cullingRadius = radius + std::max(frustumMargin, 0.0f);
	glm::mat4 clip = m_projectionMatrix * m_viewMatrix;
	auto row = [&](int index)
	{
		return glm::vec4(clip[0][index], clip[1][index], clip[2][index], clip[3][index]);
	};
	glm::vec4 rows[4] = { row(0), row(1), row(2), row(3) };
	glm::vec4 planes[6] =
	{
		rows[3] + rows[0], rows[3] - rows[0],
		rows[3] + rows[1], rows[3] - rows[1],
		rows[3] + rows[2], rows[3] - rows[2]
	};
	for (glm::vec4 plane : planes)
	{
		float normalLength = glm::length(glm::vec3(plane));
		if (normalLength <= 0.00001f)
		{
			continue;
		}
		plane /= normalLength;
		if (glm::dot(glm::vec3(plane), center) + plane.w < -cullingRadius)
		{
			return false;
		}
	}
	return true;
}
void pwg::Renderer::DrawAll()
{
	if (m_debugSettings.wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	DrawSkyBox();

	for (auto& r : m_renderQueue)
	{
		
		m_lightingUploader->Upload(r->GetMaterial(), m_environmentLighting, m_cameraPosition);

		r->GetMaterial()->SetUniformMat4("u_view", m_viewMatrix);
		r->GetMaterial()->SetUniformMat4("u_projection", m_projectionMatrix);
		r->GetMaterial()->SetUniformMat4("u_lightSpaceMatrix0", m_lightSpaceMatrices[0]);
		r->GetMaterial()->SetUniformMat4("u_lightSpaceMatrix1", m_lightSpaceMatrices[1]);
		r->GetMaterial()->SetUniformMat4("u_lightSpaceMatrix2", m_lightSpaceMatrices[2]);
		r->GetMaterial()->SetUniformInt("u_shadowsEnabled", (m_shadowMapEnabled && m_environmentLighting.shadowsEnabled) ? 1 : 0);
		r->GetMaterial()->SetUniformFloat("u_shadowStrength", m_environmentLighting.shadowStrength);
		r->GetMaterial()->SetUniformFloat("u_shadowBias", m_environmentLighting.shadowBias);
		r->GetMaterial()->SetUniformFloat("u_shadowNormalBias", m_environmentLighting.shadowNormalBias);
		r->GetMaterial()->SetUniformFloat("u_shadowDistance", m_environmentLighting.shadowDistance);
		r->GetMaterial()->SetUniformVec3("u_shadowCascadeSplits", glm::vec3(m_shadowCascadeSplits[0], m_shadowCascadeSplits[1], m_shadowCascadeSplits[2]));

		r->GetMaterial()->Apply();
		auto shader = r->GetMaterial()->GetShader();
		if (shader && m_shadowTextureIDs[0] != 0)
		{
			const int firstShadowSlot = 12;
			for (int i = 0; i < 3; ++i)
			{
				glActiveTexture(GL_TEXTURE0 + firstShadowSlot + i);
				glBindTexture(GL_TEXTURE_2D, m_shadowTextureIDs[i]);
			}
			shader->SetUniformInt("u_shadowMap0", firstShadowSlot);
			shader->SetUniformInt("u_shadowMap1", firstShadowSlot + 1);
			shader->SetUniformInt("u_shadowMap2", firstShadowSlot + 2);
		}

		r->Draw(*this);
	}
}

void pwg::Renderer::DrawSkyBox()
{
	if (!m_skybox)
	{
		return;
	}

	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);

	glm::mat4 view = glm::mat4(glm::mat3(m_viewMatrix));

	m_skybox->GetMaterial()->SetUniformMat4("u_view", view);
	m_skybox->GetMaterial()->SetUniformMat4("u_projection", m_projectionMatrix);
	m_skybox->GetMaterial()->SetUniformVec3("u_sunDirection", m_environmentLighting.sunDirection);
	m_skybox->GetMaterial()->SetUniformVec3("u_sunColor", m_environmentLighting.sunColor);
	m_skybox->GetMaterial()->SetUniformFloat("u_sunIntensity", m_environmentLighting.sunIntensity);
	m_skybox->GetMaterial()->SetUniformFloat("u_sunAngularSize", m_environmentLighting.sunAngularSize);
	m_skybox->GetMaterial()->SetUniformVec3("u_skyTint", m_environmentLighting.skyTint);
	m_skybox->GetMaterial()->SetUniformVec3("u_groundAlbedo", m_environmentLighting.groundAlbedo);
	m_skybox->GetMaterial()->SetUniformFloat("u_atmosphereDensity", m_environmentLighting.atmosphereDensity);
	m_skybox->GetMaterial()->SetUniformFloat("u_haze", m_environmentLighting.haze);
	m_skybox->GetMaterial()->SetUniformInt("u_useSkybox", m_environmentLighting.useSkyboxBlend ? 1 : 0);
	m_skybox->GetMaterial()->SetUniformFloat("u_skyboxBlend", m_environmentLighting.skyboxBlend);

	m_skybox->GetMaterial()->Apply();

	m_skybox->Draw();

	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS); 
}

void pwg::Renderer::AddToQueue(IRenderable* renderable)
{
	m_renderQueue.push_back(renderable);
}

void pwg::Renderer::SetEnvironmentLighting(const EnvironmentLighting& lighting)
{
	m_environmentLighting = lighting;
}

void pwg::Renderer::SetShadowMaps(const std::array<glm::mat4, 3>& lightSpaceMatrices, const std::array<unsigned int, 3>& shadowTextureIDs, const std::array<float, 3>& cascadeSplits, bool enabled)
{
	m_lightSpaceMatrices = lightSpaceMatrices;
	m_shadowTextureIDs = shadowTextureIDs;
	m_shadowCascadeSplits = cascadeSplits;
	m_shadowMapEnabled = enabled;
}

void pwg::Renderer::AddSkyBox(std::shared_ptr<SkyBox> skybox)
{
	m_skybox = skybox;
}

void pwg::Renderer::ClearQueue()
{
	m_renderQueue.clear();
}

void pwg::Renderer::SetCamera(std::shared_ptr<ICamera> camera)
{
	m_viewMatrix = camera->GetViewMatrix();
	m_projectionMatrix = camera->GetProjectionMatrix();
	m_cameraPosition = camera->GetCameraPosition();
}

void pwg::Renderer::SetDebugSettings(DebugSettings& settings)
{
	
	m_debugSettings = settings;
}



