#include <glad/glad.h>
#include "renderer.h"
#include "core/logger/logger.h"

pwg::Renderer::Renderer(std::shared_ptr<ResourceManager> resourceManager)
	:m_resourceManager(resourceManager)
{
	auto& shaderManager = m_resourceManager->GetShaderManager();
	shaderManager.Load("default", "../assets/shaders/default.vert", "../assets/shaders/default.frag");
	shaderManager.LoadComputeWithInclude("noise", "../assets/shaders/noise.comp", "../assets/shaders/FastNoiseLite.glsl");

	auto& textureManager = m_resourceManager->GetTextureManager();
	textureManager.Load("dirt", "../assets/textures/dirt.png");
	m_currentShader = m_resourceManager->GetShaderManager().GetShader<pwg::Shader>("default");
	glEnable(GL_DEPTH_TEST);

	PWG_INFO("Renderer initialized");
}

pwg::Renderer::~Renderer()
{

}

void pwg::Renderer::Clear()
{
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void pwg::Renderer::Update(pwg::components::CameraComponent* camera, Mesh& mesh)
{
	m_currentShader->ActivateShader();
	
	if (camera)
	{
		m_currentShader->SetUniformMat4("view", camera->viewMatrix);
		m_currentShader->SetUniformMat4("projection", camera->projectionMatrix);
	}

	mesh.Update(m_currentShader->GetShaderID());
}

void pwg::Renderer::Draw(Terrain& terrain) 
{
	
	terrain.Draw(*m_currentShader, *m_resourceManager->GetShaderManager().GetShader<pwg::ComputeShader>("noise"));

}
