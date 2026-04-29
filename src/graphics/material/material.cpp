#include "material.h"

namespace pwg
{
	Material::Material(std::shared_ptr<Shader> shader)
		: m_shader(shader)
	{

	}

	Material::~Material()
	{
	}

	void Material::Apply()
	{

		m_shader->ActivateShader();

		for (auto& [name, value] : m_floatUniforms)
		{
			m_shader->SetUniformFloat(name, value);
		}

		for (auto& [name, value] : m_intUniforms)
		{
			m_shader->SetUniformInt(name, value);
		}

		for (auto& [name, value] : m_vec2Uniforms)
		{
			m_shader->SetUniformVec2(name, value);
		}

		for (auto& [name, value] : m_vec3Uniforms)
		{
			m_shader->SetUniformVec3(name, value);
		}

		for (auto& [name, value] : m_mat4Uniforms)
		{
			m_shader->SetUniformMat4(name, value);
		}

		int slot = 0;
		for (auto& [name, tex] : m_textureUniforms)
		{
			tex->Bind(slot);
			m_shader->SetUniformInt(name, slot);
			slot++;
		}

		for (auto& [name, cubemap] : m_cubeMapTextures)
		{
			cubemap->Bind(slot);
			m_shader->SetUniformInt(name, slot);
			slot++;
		}

		for (auto& [name, texArray] : m_textureArrays)
		{
			texArray->Bind(slot);
			m_shader->SetUniformInt(name, slot);
			slot++;
		}


	}

	float Material::GetUniformFloat(const std::string& name)
	{
		return m_floatUniforms[name];
	}

	std::shared_ptr<Texture> Material::GetUniformTexture(const std::string& name)
	{
		return m_textureUniforms[name];
	}

	int Material::GetUniformInt(const std::string& name)
	{
		return m_intUniforms[name];
	}

	glm::vec3 Material::GetUniformVec3(const std::string& name)
	{
		return m_vec3Uniforms[name];
	}

	glm::mat4 Material::GetUniformMat4(const std::string& name)
	{
		return m_mat4Uniforms[name];
	}

	MaterialProperties& Material::GetMaterialProperties()
	{
		return m_materialProperties;
	}

	void Material::SetShader(std::shared_ptr<Shader> shader)
	{
		m_shader = shader;
	}

	void Material::SetUniformFloat(const std::string& name, float value)
	{
		m_floatUniforms[name] = value;
	}

	void Material::SetUniformTexture(const std::string& name, std::shared_ptr<Texture> texture)
	{
		m_textureUniforms[name] = texture;
	}

	void Material::SetUniformInt(const std::string& name, int value)
	{
		m_intUniforms[name] = value;
	}

	void Material::SetUniformVec2(const std::string& name, const glm::vec2& value)
	{
		m_vec2Uniforms[name] = value;
	}

	void Material::SetUniformVec3(const std::string& name, const glm::vec3& value)
	{
		m_vec3Uniforms[name] = value;
	}

	void Material::SetUniformMat4(const std::string& name, const glm::mat4& value)
	{
		m_mat4Uniforms[name] = value;
	}

	void Material::SetTextureArray(const std::string& name, std::shared_ptr<TextureArray> textureArray)
	{
		m_textureArrays[name] = textureArray;
	}

	void Material::SetCubeMapTexture(const std::string& name, std::shared_ptr<CubeMapTexture> cubeMapTexture)
	{
		m_cubeMapTextures[name] = cubeMapTexture;
	}

	void Material::SetMaterialProperties(MaterialProperties& materialProperties)
	{
		m_materialProperties.albedo = materialProperties.albedo;
		m_materialProperties.ao = materialProperties.ao;
		m_materialProperties.rougness = materialProperties.rougness;
		m_materialProperties.metallic = materialProperties.metallic;
		m_materialProperties.heightmap = materialProperties.heightmap;
		m_materialProperties.normalmap = materialProperties.normalmap;
	}

}


