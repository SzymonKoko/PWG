#ifndef SRC_GRAPHICS_SHADER_SHADER_H_
#define SRC_GRAPHICS_SHADER_SHADER_H_

#include <glad.h>
#include <iostream>
#include <fstream>
#include <string>

#include "core/logger/logger.h"

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
		Shader(const char* vertexFilePath, const char* fragmentFilePath);
		~Shader();


		/**
		 * @brief Reads shader code from file
		 * @param shaderPath Path to the shader file
		 */
		std::string ReadFromShaderFile(const char* shaderPath);


		/**
		 * @brief Activates shader program
		 */
		void ActivateShader();


		/**
		 * @brief Deletes shader program
		 */
		void DeleteShader();


		/**
		 * @brief Returns shader program ID
		 */
		unsigned int GetShaderID();
	private:
		GLuint m_shaderID{ 0 };
	};
}
#endif // !SRC_GRAPHICS_SHADER_SHADER_H_
