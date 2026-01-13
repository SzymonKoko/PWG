#include <glad/glad.h>
#include "renderer.h"
#include "core/logger/logger.h"
#include "graphics/renderer/irenderable.h"

pwg::Renderer::Renderer()
{
	/*auto& shaderManager = m_resourceManager->GetShaderManager();
	shaderManager.Load("default", "../assets/shaders/default.vert", "../assets/shaders/default.frag");
	shaderManager.LoadComputeWithInclude("noise", "../assets/shaders/noise.comp", "../assets/shaders/FastNoiseLite.glsl");

	auto& textureManager = m_resourceManager->GetTextureManager();
	textureManager.Load("dirt", "../assets/textures/dirt.png");
	m_currentShader = m_resourceManager->GetShaderManager().GetShader<pwg::Shader>("default");*/
	

	PWG_INFO("Renderer initialized");
}

pwg::Renderer::~Renderer()
{

}

void pwg::Renderer::BeginFrame()
{
	glEnable(GL_DEPTH_TEST);

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_renderQueue.clear();
}

void pwg::Renderer::EndFrame()
{
}

void pwg::Renderer::Draw(IRenderable* renderable)
{
	renderable->Draw(*this);
}

void pwg::Renderer::DrawAll()
{
	for (auto& r : m_renderQueue)
	{
		m_lightingUploader->Upload(r->GetMaterial(), m_light, m_cameraPosition);
		r->GetMaterial()->Apply();

		r->GetMaterial()->SetUniformMat4("u_view", m_viewMatrix);
		r->GetMaterial()->SetUniformMat4("u_projection", m_projectionMatrix);


		r->Draw(*this);
	}
}

void pwg::Renderer::AddToQueue(IRenderable* renderable)
{
	m_renderQueue.push_back(renderable);
}

void pwg::Renderer::AddLight(Light& light)
{
	m_light = light;
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


