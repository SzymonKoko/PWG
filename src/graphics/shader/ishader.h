#ifndef SRC_GRAPHICS_SHADER_ISHADER_H_
#define SRC_GRAPHICS_SHADER_ISHADER_H_

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

#include "core/logger/logger.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace pwg
{
	/**
	* @brief Interface for OpenGL shader programs.
	* Defines methods for shader creation, activation, deletion, and uniform management.
	*/
	class IShader
	{
	public:

		/**
		* @brief Virtual destructor. Ensures derived shaders clean up resources.
		*/
		virtual ~IShader() = default;

		/**
		* @brief Reads shader source code from a file.
		* @param shaderPath Path to the shader file.
		* @return Shader source code as a string.
		*/
		virtual std::string ReadFromShaderFile(const std::filesystem::path& shaderPath) = 0;

		/**
		* @brief Preprocesses shader source (e.g., handling includes or macros).
		* @param shaderSource Raw shader code.
		* @param filePath Path to the shader file (for relative includes).
		* @return Processed shader source code.
		*/
		virtual std::string PreprocessShader(const std::string& shaderSource, const std::filesystem::path& filePath) = 0;

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
		* @param name Uniform variable name.
		* @param value Boolean value to assign.
		*/
		virtual void SetUniformBool(const std::string& name, bool value) const = 0;

		/**
		* @brief Sets an integer uniform in the shader.
		* @param name Uniform variable name.
		* @param value Integer value to assign.
		*/
		virtual void SetUniformInt(const std::string& name, int value) const = 0;

		/**
		* @brief Sets an vector of integers uniform in the shader.
		* @param name Uniform variable name.
		* @param value Vector of integers to assign.
		*/
		virtual void SetUniformVectorInt(const std::string& name, std::vector<int> value) const = 0;

		/**
		* @brief Sets a float uniform in the shader.
		* @param name Uniform variable name.
		* @param value Float value to assign.
		*/
		virtual void SetUniformFloat(const std::string& name, float value) const = 0;

		/**
		* @brief Sets an vector of floats uniform in the shader.
		* @param name Uniform variable name.
		* @param value Vector of floats to assign.
		*/
		virtual void SetUniformVectorFloat(const std::string& name, std::vector<float> value) const = 0;

		/**
		* @brief Sets a mat4 uniform in the shader.
		* @param name Uniform variable name.
		* @param value 4x4 matrix to assign.
		*/
		virtual void SetUniformMat4(const std::string& name, glm::mat4 value) const = 0;

		/**
		* @brief Sets a vec2 uniform in the shader.
		* @param name Uniform variable name.
		* @param value 2D vector to assign.
		*/
		virtual void SetUniformVec2(const std::string& name, glm::vec2 value) const = 0;

		/**
		* @brief Sets a vec3 uniform in the shader.
		* @param name Uniform variable name.
		* @param value 3D vector to assign.
		*/
		virtual void SetUniformVec3(const std::string& name, glm::vec3 value) const = 0;

		/**
		* @brief Returns the location of a uniform variable.
		* @param uniformName Name of the uniform.
		* @return OpenGL uniform location ID.
		*/
		virtual int GetUniformLocation(const std::string& uniformName) = 0;

		/**
		* @brief Retrieves the integer value of a uniform variable.
		* @param uniformName Name of the uniform.
		* @return Integer value of the uniform.
		*/
		virtual int GetUniformInt(const std::string& uniformName) const = 0;

		/**
		* @brief Retrieves the float value of a uniform variable.
		* @param uniformName Name of the uniform.
		* @return Float value of the uniform.
		*/
		virtual float GetUniformFloat(const std::string& uniformName) const = 0;

		/**
		* @brief Retrieves the mat4 value of a uniform variable.
		* @param uniformName Name of the uniform.
		* @return 4x4 matrix value of the uniform.
		*/
		virtual glm::mat4 GetUniformMat4(const std::string& uniformName) const = 0;

		/**
		* @brief Retrieves the vec2 value of a uniform variable.
		* @param uniformName Name of the uniform.
		* @return 2D vector value of the uniform.
		*/
		virtual glm::vec2 GetUniformVec2(const std::string& uniformName) const = 0;

		/**
		* @brief Retrieves the vec3 value of a uniform variable.
		* @param uniformName Name of the uniform.
		* @return 3D vector value of the uniform.
		*/
		virtual glm::vec3 GetUniformVec3(const std::string& uniformName) const = 0;

		/**
		* @brief Returns the OpenGL shader program ID.
		* @return Shader program ID.
		*/
		virtual unsigned int GetShaderID() = 0;
	};
} // namespace pwg

#endif // !SRC_GRAPHICS_SHADER_ISHADER_H_
