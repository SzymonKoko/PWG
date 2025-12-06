#ifndef SRC_GRAPHICS_MATERIAL_MATERIAL_H_
#define SRC_GRAPHICS_MATERIAL_MATERIAL_H_

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include "graphics/shader/shader.h"
#include "graphics/texture/texture.h"


namespace pwg
{
	class Material
	{
	public:
		Material(Shader& shader);
		~Material();

		void Apply();

		//Gettery, Settery
		float GetUniformFLoat(const std::string& name);
		std::shared_ptr<Texture> GetUniformTexture(const std::string& name);
		int GetUniformInt(const std::string& name);
		glm::vec3 GetUniformVec3(const std::string& name);

		void SetShader(Shader& shader);
		void SetUniformFLoat(const std::string& name, float value);
		void SetUniformTexture(const std::string& name, std::shared_ptr<Texture> texture, int slot);
		void SetUniformInt(const std::string& name, int value);
		void SetUniformVec3(const std::string& name, const glm::vec3& value);
	private:
		
		struct TextureData
		{
			std::shared_ptr<Texture> texture;
			int slot;
		};

		std::unordered_map<std::string, float> m_floatUniforms;
		std::unordered_map<std::string, TextureData> m_textureUniforms;
		std::unordered_map<std::string, int> m_intUniforms;
		std::unordered_map<std::string, glm::vec3> m_vec3Uniforms;

		

		Shader& m_shader;


	};
}

#endif // !SRC_GRAPHICS_MATERIAL_MATERIAL_H_

//SetShader