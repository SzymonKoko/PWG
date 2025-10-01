#include <glad/glad.h>
#include "renderer.h"

pwg::Renderer::Renderer()
{
	InitShaders();
	InitTextures();
	glEnable(GL_DEPTH_TEST);
}

pwg::Renderer::~Renderer()
{
	m_shaderProgram->DeleteShader();
}

void pwg::Renderer::Clear()
{
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void pwg::Renderer::Update(pwg::components::CameraComponent* camera)
{
	m_shaderProgram->ActivateShader();
	
	if (camera)
	{
		m_shaderProgram->SetUniformMat4("view", camera->viewMatrix);
		m_shaderProgram->SetUniformMat4("projection", camera->projectionMatrix);
	}

	m_pyramidMesh.Update(GetShaderProgramID());
}

void pwg::Renderer::Draw() 
{
	m_texDirt.Bind();
	m_pyramidMesh.Bind();

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElementsInstanced(GL_TRIANGLES, m_pyramidMesh.GetIndexCount(), GL_UNSIGNED_INT, 0, m_pyramidMesh.GetInstancedPositionsSize());
	
	m_pyramidMesh.Unbind();
	m_texDirt.Unbind();

}

void pwg::Renderer::InitShaders()
{
	m_shaderProgram = new Shader("../assets/shaders/default.vert", "../assets/shaders/default.frag");
	m_shaderProgram->ActivateShader();
	m_shaderProgram->SetUniformInt("Texture", 0);
}

void pwg::Renderer::InitTextures()
{
	m_texDirt.LoadFromFile("../assets/textures/texture.png");
}


GLuint pwg::Renderer::GetShaderProgramID()
{
	return m_shaderProgram->GetShaderID();
}
