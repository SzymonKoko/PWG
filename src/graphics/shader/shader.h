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
	enum ShaderType
	{
		DEFAULT = 0,
		STANDARD,
		COMPUTE,
		COUNT
	};

	/**
	 * @brief Class responsible for creating, activating, and deleting OpenGL shaders.
	 * Provides convenient methods to set shader uniforms of various types.
	 */
	class Shader
	{
	public:

		/**
		 * @brief Constructs a shader program from vertex and fragment shader files.
		 * Compiles and links shaders into a single program.
		 * @param vertexFilePath Path to the vertex shader file.
		 * @param fragmentFilePath Path to the fragment shader file.
		 */
		Shader(const std::string vertexFilePath, const std::string fragmentFilePath);

		/**
		 * @brief Constructs a shader program from compute shader files.
		 * Compiles and links shaders into a single program.
		 * @param computeFilePath Path to the compute shader file.
		 */
		Shader(const std::string computeFilePath);

		/**
		 * @brief Destructor. Deletes the shader program.
		 */
		~Shader();

		/**
		 * @brief Reads shader code from a file.
		 * @param shaderPath Path to the shader file.
		 * @return Shader source code as a string.
		 */
		std::string ReadFromShaderFile(const std::string shaderPath);

		/**
		 * @brief Activates the shader program for rendering.
		 */
		void ActivateShader();

		/**
		 * @brief Deletes the shader program from GPU memory.
		 */
		void DeleteShader();

		/**
		 * @brief Sets a boolean uniform in the shader.
		 * @param name Name of the uniform variable in the shader.
		 * @param value Boolean value to set.
		 */
		void SetUniformBool(const std::string& name, bool value) const;

		/**
		 * @brief Sets an integer uniform in the shader.
		 * @param name Name of the uniform variable in the shader.
		 * @param value Integer value to set.
		 */
		void SetUniformInt(const std::string& name, int value) const;

		/**
		 * @brief Sets a float uniform in the shader.
		 * @param name Name of the uniform variable in the shader.
		 * @param value Float value to set.
		 */
		void SetUniformFloat(const std::string& name, float value) const;

		/**
		 * @brief Sets a mat4 uniform in the shader.
		 * @param name Name of the uniform variable in the shader.
		 * @param value 4x4 matrix value to set.
		 */
		void SetUniformMat4(const std::string& name, glm::mat4 value) const;

		/**
		 * @brief Sets a vec3 uniform in the shader.
		 * @param name Name of the uniform variable in the shader.
		 * @param value 3D vector value to set.
		 */
		void SetUniformVec3(const std::string& name, glm::vec3 value) const;

		/**
		 * @brief Returns the OpenGL uniform location ID.
		 * @param uniformName Name of the uniform.
		 * @return Uniform location ID.
		 */
		int GetUniformLocation(const std::string& uniformName);

		/**
		 * @brief Returns the OpenGL shader program ID.
		 * @return Shader program ID.
		 */
		unsigned int GetShaderID();

	private:
		unsigned int m_shaderID{ 0 }; /**< OpenGL shader program ID. */
		std::unordered_map<std::string, int> m_uniformLocations;
		ShaderType m_shaderType{ DEFAULT };

	};
} // namespace pwg

#endif // !SRC_GRAPHICS_SHADER_SHADER_H_
