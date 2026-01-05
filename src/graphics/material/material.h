#ifndef SRC_GRAPHICS_MATERIAL_MATERIAL_H_
#define SRC_GRAPHICS_MATERIAL_MATERIAL_H_

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include "graphics/shader/shader.h"
#include "graphics/texture/texture.h"
#include "graphics/texture/textureArray.h"


namespace pwg
{
	struct MaterialProperties
	{
		glm::vec3 diffuse{ 0.0f };
		glm::vec3 ambient{ 0.0f };
		glm::vec3 specular{ 0.0f };
		float shininess{ 0.0f };
	};

	class Material
	{
	public:
		Material(std::shared_ptr<Shader> shader);
		~Material();

		void Apply();

		//Gettery, Settery
		float GetUniformFloat(const std::string& name);
		std::shared_ptr<Texture> GetUniformTexture(const std::string& name);
		int GetUniformInt(const std::string& name);
		glm::vec3 GetUniformVec3(const std::string& name);
		glm::mat4 GetUniformMat4(const std::string& name);
		MaterialProperties& GetMaterialProperties();

		void SetShader(std::shared_ptr<Shader> shader);
		void SetUniformFloat(const std::string& name, float value);
		void SetUniformTexture(const std::string& name, std::shared_ptr<Texture> texture, int slot);
		void SetUniformInt(const std::string& name, int value);
		void SetUniformVec3(const std::string& name, const glm::vec3& value);
		void SetUniformMat4(const std::string& name, const glm::mat4& value);
		void SetTextureArray(const std::string& name, std::shared_ptr<TextureArray> textureArray);
		void SetMaterialProperties(MaterialProperties& materialProperties);

	private:
		
		struct TextureData
		{
			std::shared_ptr<Texture> texture{ nullptr };
			int slot{ 0 };
		};

		std::unordered_map<std::string, float> m_floatUniforms;
		std::unordered_map<std::string, TextureData> m_textureUniforms;
		std::unordered_map<std::string, int> m_intUniforms;
		std::unordered_map<std::string, glm::vec3> m_vec3Uniforms;
		std::unordered_map<std::string, glm::mat4> m_mat4Uniforms;
		std::unordered_map<std::string, std::shared_ptr<TextureArray>> m_textureArrays;
		

		std::shared_ptr<Shader> m_shader; 
		MaterialProperties m_materialProperties;


	};
}

#endif // !SRC_GRAPHICS_MATERIAL_MATERIAL_H_

//SetShader