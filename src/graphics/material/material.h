#ifndef SRC_GRAPHICS_MATERIAL_MATERIAL_H_
#define SRC_GRAPHICS_MATERIAL_MATERIAL_H_

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include "graphics/shader/shader.h"
#include "graphics/texture/texture.h"
#include "graphics/texture/textureArray.h"
#include "graphics/texture/cubeMapTexture.h"

namespace pwg
{
	/**
	* @brief Structure storing basic material lighting properties.
	* Commonly used in lighting calculations such as Phong or Blinn-Phong shading.
	*/
	struct MaterialProperties
	{
		glm::vec3 diffuse{ 0.0f };	/**< Diffuse color component */
		glm::vec3 ambient{ 0.0f };	/**< Ambient color component */
		glm::vec3 specular{ 0.0f };	/**< Specular color component */
		float shininess{ 0.0f };	/**< Specular shininess factor */

		std::shared_ptr<Texture> albedo;
		std::shared_ptr<Texture> heightmap;
		std::shared_ptr<Texture> normalmap;
		std::shared_ptr<Texture> ao;
		std::shared_ptr<Texture> rougness;
		std::shared_ptr<Texture> metallic;
	};

	/**
	* @brief Represents a render material.
	* Manages shader assignment, uniform values, textures, and material properties.
	*/
	class Material
	{
	public:
		/**
		* @brief Constructs a material with a given shader.
		* @param shader Shared pointer to the shader used by this material.
		*/
		Material(std::shared_ptr<Shader> shader);

		/**
		* @brief Destructor for the material.
		*/
		~Material();

		/**
		* @brief Applies the material.
		* Binds the shader and uploads all stored uniforms and textures.
		*/
		void Apply();

		// Getters / setters

		/**
		* @brief Retrieves a float uniform value by name.
		* @param name Uniform name.
		* @return Stored float value.
		*/
		float GetUniformFloat(const std::string& name);

		/**
		* @brief Retrieves a texture uniform by name.
		* @param name Uniform name.
		* @return Shared pointer to the texture.
		*/
		std::shared_ptr<Texture> GetUniformTexture(const std::string& name);

		/**
		* @brief Retrieves an integer uniform value by name.
		* @param name Uniform name.
		* @return Stored integer value.
		*/
		int GetUniformInt(const std::string& name);

		/**
		* @brief Retrieves a vec3 uniform value by name.
		* @param name Uniform name.
		* @return Stored glm::vec3 value.
		*/
		glm::vec3 GetUniformVec3(const std::string& name);

		/**
		* @brief Retrieves a mat4 uniform value by name.
		* @param name Uniform name.
		* @return Stored glm::mat4 value.
		*/
		glm::mat4 GetUniformMat4(const std::string& name);

		/**
		* @brief Returns a reference to the material lighting properties.
		* @return Reference to MaterialProperties structure.
		*/
		MaterialProperties& GetMaterialProperties();

		/**
		* @brief Sets the shader used by this material.
		* @param shader Shared pointer to the new shader.
		*/
		void SetShader(std::shared_ptr<Shader> shader);

		/**
		* @brief Sets a float uniform value.
		* @param name Uniform name.
		* @param value Float value to assign.
		*/
		void SetUniformFloat(const std::string& name, float value);

		/**
		* @brief Sets a texture uniform and assigns it to a texture slot.
		* @param name Uniform name.
		* @param texture Texture to bind.
		* @param slot Texture slot index.
		*/
		void SetUniformTexture(const std::string& name, std::shared_ptr<Texture> texture);

		/**
		* @brief Sets an integer uniform value.
		* @param name Uniform name.
		* @param value Integer value to assign.
		*/
		void SetUniformInt(const std::string& name, int value);

		/**
		* @brief Sets a vec2 uniform value.
		* @param name Uniform name.
		* @param value glm::vec2 value to assign.
		*/
		void SetUniformVec2(const std::string& name, const glm::vec2& value);

		/**
		* @brief Sets a vec3 uniform value.
		* @param name Uniform name.
		* @param value glm::vec3 value to assign.
		*/
		void SetUniformVec3(const std::string& name, const glm::vec3& value);

		/**
		* @brief Sets a mat4 uniform value.
		* @param name Uniform name.
		* @param value glm::mat4 value to assign.
		*/
		void SetUniformMat4(const std::string& name, const glm::mat4& value);

		/**
		* @brief Sets a texture array uniform.
		* @param name Uniform name.
		* @param textureArray Texture array to bind.
		*/
		void SetTextureArray(const std::string& name, std::shared_ptr<TextureArray> textureArray);

		void SetCubeMapTexture(const std::string& name, std::shared_ptr<CubeMapTexture> cubeMapTexture);

		/**
		* @brief Updates the material lighting properties.
		* @param materialProperties New material properties.
		*/
		void SetMaterialProperties(MaterialProperties& materialProperties);

	private:

		std::unordered_map<std::string, float> m_floatUniforms;								/**< Stored float uniforms */
		std::unordered_map<std::string, std::shared_ptr<Texture>> m_textureUniforms;		/**< Stored texture uniforms */
		std::unordered_map<std::string, int> m_intUniforms;									/**< Stored integer uniforms */
		std::unordered_map<std::string, glm::vec2> m_vec2Uniforms;							/**< Stored vec2 uniforms */
		std::unordered_map<std::string, glm::vec3> m_vec3Uniforms;							/**< Stored vec3 uniforms */
		std::unordered_map<std::string, glm::mat4> m_mat4Uniforms;							/**< Stored mat4 uniforms */
		std::unordered_map<std::string, std::shared_ptr<TextureArray>> m_textureArrays;		/**< Stored texture arrays */
		std::unordered_map<std::string, std::shared_ptr<CubeMapTexture>> m_cubeMapTextures;	/**< Stored cube map textures */

		std::shared_ptr<Shader> m_shader;													/**< Shader used by the material */
		MaterialProperties m_materialProperties;											/**< Material lighting properties */
	};
}

#endif // !SRC_GRAPHICS_MATERIAL_MATERIAL_H_
