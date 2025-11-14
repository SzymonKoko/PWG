#include <glad/glad.h>
#include "renderer.h"
#include "core/logger/logger.h"

pwg::Renderer::Renderer(std::shared_ptr<ResourceManager> resourceManager)
	:m_resourceManager(resourceManager)
{
	auto& shaderManager = m_resourceManager->GetShaderManager();
	shaderManager.Load("default", "../assets/shaders/default.vert", "../assets/shaders/default.frag");

	auto& textureManager = m_resourceManager->GetTextureManager();
	textureManager.Load("dirt", "../assets/textures/dirt.png");
	m_currentShader = m_resourceManager->GetShaderManager().GetShader("default");
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
	
	//std::cout << "Shader ID: " << shader.GetShaderID() << std::endl;
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
	m_currentShader->ActivateShader();
	
	terrain.Draw(*m_currentShader);

}
