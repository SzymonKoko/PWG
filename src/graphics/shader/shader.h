#ifndef SRC_GRAPHICS_SHADER_SHADER_H_
#define SRC_GRAPHICS_SHADER_SHADER_H_

#include <iostream>
#include <fstream>
#include <string>

#include "core/logger/logger.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace pwg
{
	/**
	 * @brief Creating, activating and deleteing shaders 
	 */
	class Shader
	{
	public:

		/**
		 * @brief Shader constructor
		 * @param vertexFilePath Indicates path to the vertex shader code file
		 * @param fragmentFilePath Indicates path to the fragment shader code file
		 */
		Shader(const std::string vertexFilePath, const std::string fragmentFilePath);
		~Shader();


		/**
		 * @brief Reads shader code from file
		 * @param shaderPath Path to the shader file
		 */
		std::string ReadFromShaderFile(const std::string shaderPath);


		/**
		 * @brief Activates shader program
		 */
		void ActivateShader();


		/**
		 * @brief Deletes shader program
		 */
		void DeleteShader();


		/**
		 * @brief Sets shader uniform to bool value
		 * @param name Name of the uniform
		 * @param value Boolean value for the uniform
		 */
		void SetUniformBool(const std::string& name, bool value) const;


		/**
		 * @brief Sets shader uniform to int value
		 * @param name Name of the uniform
		 * @param value Integer value for the uniform
		 */
		void SetUniformInt(const std::string& name, int value) const;


		/**
		 * @brief Sets shader uniform to float value
		 * @param name Name of the uniform
		 * @param value Float value for the uniform
		 */
		void SetUniformFloat(const std::string& name, float value) const;

		/**
		 * @brief Sets shader uniform to mat4 value
		 * @param name Name of the uniform
		 * @param value Mat4 value for the uniform
		 */
		void SetUniformMat4(const std::string& name, glm::mat4 value) const;

		/**
		 * @brief Sets shader uniform to vec3 value
		 * @param name Name of the uniform
		 * @param value Vec3 value for the uniform
		 */
		void SetUniformVec3(const std::string& name, glm::vec3 value) const;


		/**
		 * @brief Returns shader program ID
		 */
		unsigned int GetShaderID();
	private:
		unsigned int m_shaderID{ 0 };
	};
}
#endif // !SRC_GRAPHICS_SHADER_SHADER_H_
