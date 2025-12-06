#include "material.h"

namespace pwg
{
	Material::Material(Shader& shader)
		: m_shader(shader)
	{

	}

	Material::~Material()
	{
	}

	void Material::Apply()
	{
		for (auto& [name, value] : m_floatUniforms)
		{
			m_shader.SetUniformFloat(name, value);
		}

		for (auto& [name, value] : m_intUniforms)
		{
			m_shader.SetUniformInt(name, value);
		}

		for (auto& [name, value] : m_vec3Uniforms)
		{
			m_shader.SetUniformVec3(name, value);
		}
		for (auto& [name, texData] : m_textureUniforms)
		{
			texData.texture->Bind(texData.slot);
			m_shader.SetUniformInt(name, texData.slot);
		}


	}

	float Material::GetUniformFLoat(const std::string& name)
	{
		return m_floatUniforms[name];
	}

	std::shared_ptr<Texture> Material::GetUniformTexture(const std::string& name)
	{
		return m_textureUniforms[name].texture;
	}

	int Material::GetUniformInt(const std::string& name)
	{
		return m_intUniforms[name];
	}

	glm::vec3 Material::GetUniformVec3(const std::string& name)
	{
		return m_vec3Uniforms[name];
	}

	void Material::SetShader(Shader& shader)
	{
		m_shader = shader;
	}

	void Material::SetUniformFLoat(const std::string& name, float value)
	{
		m_floatUniforms[name] = value;
	}

	void Material::SetUniformTexture(const std::string& name, std::shared_ptr<Texture> texture, int slot)
	{
		m_textureUniforms[name] = TextureData(texture, slot);
	}

	void Material::SetUniformInt(const std::string& name, int value)
	{
		m_intUniforms[name] = value;
	}

	void Material::SetUniformVec3(const std::string& name, const glm::vec3& value)
	{
		m_vec3Uniforms[name] = value;
	}

}


