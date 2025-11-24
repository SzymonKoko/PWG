#ifndef SRC_GRAPHICS_SHADER_ISHADER_H_
#define SRC_GRAPHICS_SHADER_ISHADER_H_

#include <iostream>
#include <fstream>
#include <string>

#include "core/logger/logger.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace pwg
{

	/**
	 * @brief Class responsible for creating, activating, and deleting OpenGL shaders.
	 * Provides convenient methods to set shader uniforms of various types.
	 */
	class IShader
	{
	public:

		

		/**
		 * @brief Destructor. Deletes the shader program.
		 */
		virtual ~IShader() = default;

		/**
		 * @brief Reads shader code from a file.
		 * @param shaderPath Path to the shader file.
		 * @return Shader source code as a string.
		 */
		virtual std::string ReadFromShaderFile(const std::string& shaderPath) = 0;

		/**
		 * @brief Activates the shader program for rendering.
		 */
		virtual void ActivateShader() = 0;

		/**
		 * @brief Deletes the shader program from GPU memory.
		 */
		virtual void DeleteShader() = 0;

		/**
		 * @brief Sets a boolean uniform in the shader.
		 * @param name Name of the uniform variable in the shader.
		 * @param value Boolean value to set.
		 */
		virtual void SetUniformBool(const std::string& name, bool value) const = 0;

		/**
		 * @brief Sets an integer uniform in the shader.
		 * @param name Name of the uniform variable in the shader.
		 * @param value Integer value to set.
		 */
		virtual void SetUniformInt(const std::string& name, int value) const = 0;

		/**
		 * @brief Sets a float uniform in the shader.
		 * @param name Name of the uniform variable in the shader.
		 * @param value Float value to set.
		 */
		virtual void SetUniformFloat(const std::string& name, float value) const = 0;

		/**
		 * @brief Sets a mat4 uniform in the shader.
		 * @param name Name of the uniform variable in the shader.
		 * @param value 4x4 matrix value to set.
		 */
		virtual void SetUniformMat4(const std::string& name, glm::mat4 value) const = 0;

		/**
		 * @brief Sets a vec2 uniform in the shader.
		 * @param name Name of the uniform variable in the shader.
		 * @param value 2D vector value to set.
		 */
		virtual void SetUniformVec2(const std::string& name, glm::vec2 value) const = 0;

		/**
		 * @brief Sets a vec3 uniform in the shader.
		 * @param name Name of the uniform variable in the shader.
		 * @param value 3D vector value to set.
		 */
		virtual void SetUniformVec3(const std::string& name, glm::vec3 value) const = 0;

		/**
		 * @brief Returns the OpenGL uniform location ID.
		 * @param uniformName Name of the uniform.
		 * @return Uniform location ID.
		 */
		virtual int GetUniformLocation(const std::string& uniformName) = 0;

		/**
		 * @brief Returns the OpenGL uniform integer value.
		 * @param uniformName Name of the uniform.
		 * @return Uniform integer value.
		 */
		virtual int GetUniformInt(const std::string& uniformName) const = 0;

		/**
		 * @brief Returns the OpenGL uniform float value.
		 * @param uniformName Name of the uniform.
		 * @return Uniform float value.
		 */
		virtual float GetUniformFloat(const std::string& uniformName) const = 0;

		/**
		 * @brief Returns the OpenGL uniform mat4 value.
		 * @param uniformName Name of the uniform.
		 * @return Uniform mat4 value.
		 */
		virtual glm::mat4 GetUniformMat4(const std::string& uniformName) const = 0;

		/**
		 * @brief Returns the OpenGL uniform vec2 value.
		 * @param uniformName Name of the uniform.
		 * @return Uniform vec2 value.
		 */
		virtual glm::vec2 GetUniformVec2(const std::string& uniformName) const = 0;

		/**
		 * @brief Returns the OpenGL uniform vec3 value.
		 * @param uniformName Name of the uniform.
		 * @return Uniform vec3 value.
		 */
		virtual glm::vec3 GetUniformVec3(const std::string& uniformName) const = 0;

		/**
		 * @brief Returns the OpenGL shader program ID.
		 * @return Shader program ID.
		 */
		virtual unsigned int GetShaderID() = 0;

	private:


	};
} // namespace pwg

#endif // !SRC_GRAPHICS_SHADER_ISHADER_H_
