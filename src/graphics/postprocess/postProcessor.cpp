#include "postProcessor.h"

#include <glad/glad.h>
#include <glm/vec2.hpp>

namespace pwg
{
	PostProcessor::PostProcessor(std::shared_ptr<Shader> shader)
		: m_shader(shader)
	{
		InitializeQuad();
	}

	PostProcessor::~PostProcessor()
	{
		if (m_vbo != 0)
		{
			glDeleteBuffers(1, &m_vbo);
		}
		if (m_vao != 0)
		{
			glDeleteVertexArrays(1, &m_vao);
		}
	}

	void PostProcessor::Render(unsigned int sourceTexture, const EnvironmentSettings& settings, int width, int height)
	{
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_FRAMEBUFFER_SRGB);
		glViewport(0, 0, width, height);

		m_shader->ActivateShader();
		m_shader->SetUniformInt("u_scene", 0);
		m_shader->SetUniformFloat("u_exposure", settings.exposure);
		m_shader->SetUniformFloat("u_whitePoint", settings.whitePoint);
		m_shader->SetUniformFloat("u_bloomStrength", settings.bloomEnabled ? settings.bloomStrength : 0.0f);
		m_shader->SetUniformInt("u_fxaaEnabled", settings.fxaaEnabled ? 1 : 0);
		m_shader->SetUniformVec2("u_viewportSize", glm::vec2(static_cast<float>(width), static_cast<float>(height)));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, sourceTexture);
		glBindVertexArray(m_vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void PostProcessor::InitializeQuad()
	{
		float vertices[] = {
			-1.0f, -1.0f, 0.0f, 0.0f,
			 1.0f, -1.0f, 1.0f, 0.0f,
			 1.0f,  1.0f, 1.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 1.0f, 1.0f,
			-1.0f,  1.0f, 0.0f, 1.0f
		};

		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
		glBindVertexArray(0);
	}
}
