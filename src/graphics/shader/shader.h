#ifndef SRC_GRAPHICS_SHADER_SHADER_H_
#define SRC_GRAPHICS_SHADER_SHADER_H_

#include <iostream>
#include <fstream>
#include <string>
#include "ishader.h"

#include "core/logger/logger.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace pwg
{
	/**
	* @brief Class responsible for creating, managing, and using OpenGL vertex and fragment shaders.
	* Provides methods to set various uniform types and manage the shader program lifecycle.
	*/
	class Shader : public IShader
	{
	public:

		/**
		* @brief Constructs a shader program from vertex and fragment shader files.
		* Compiles and links the shaders into a single OpenGL program.
		* @param vertexFilePath Path to the vertex shader source file.
		* @param fragmentFilePath Path to the fragment shader source file.
		*/
		Shader(const std::filesystem::path& vertexFilePath, const std::filesystem::path& fragmentFilePath);

		/**
		* @brief Destructor. Deletes the shader program from GPU memory.
		*/
		~Shader();

		/**
		* @brief Reads shader source code from a file.
		* @param shaderPath Path to the shader file.
		* @return Shader source code as a string.
		*/
		std::string ReadFromShaderFile(const std::filesystem::path& shaderPath) override;

		/**
		* @brief Preprocesses shader source, e.g., handling includes or macros.
		* @param shaderSource Raw shader source code.
		* @param filePath Path to the shader file (for relative includes).
		* @return Processed shader source code.
		*/
		std::string PreprocessShader(const std::string& shaderSource, const std::filesystem::path& filePath) override;

		/**
		* @brief Activates the shader program for rendering.
		*/
		void ActivateShader() override;

		/**
		* @brief Deletes the shader program from GPU memory.
		*/
		void DeleteShader() override;

		/**
		* @brief Sets a boolean uniform variable in the shader.
		* @param name Uniform variable name.
		* @param value Boolean value to assign.
		*/
		void SetUniformBool(const std::string& name, bool value) const override;

		/**
		* @brief Sets an integer uniform variable in the shader.
		* @param name Uniform variable name.
		* @param value Integer value to assign.
		*/
		void SetUniformInt(const std::string& name, int value) const override;

		/**
		* @brief Sets a float uniform variable in the shader.
		* @param name Uniform variable name.
		* @param value Float value to assign.
		*/
		void SetUniformFloat(const std::string& name, float value) const override;

		/**
		* @brief Sets a mat4 uniform variable in the shader.
		* @param name Uniform variable name.
		* @param value 4x4 matrix value to assign.
		*/
		void SetUniformMat4(const std::string& name, glm::mat4 value) const override;

		/**
		* @brief Sets a vec2 uniform variable in the shader.
		* @param name Uniform variable name.
		* @param value 2D vector value to assign.
		*/
		void SetUniformVec2(const std::string& name, glm::vec2 value) const override;

		/**
		* @brief Sets a vec3 uniform variable in the shader.
		* @param name Uniform variable name.
		* @param value 3D vector value to assign.
		*/
		void SetUniformVec3(const std::string& name, glm::vec3 value) const override;

		/**
		* @brief Returns the OpenGL uniform location ID.
		* @param uniformName Name of the uniform.
		* @return Location ID of the uniform variable.
		*/
		int GetUniformLocation(const std::string& uniformName) override;

		/**
		* @brief Retrieves an integer uniform value from the shader.
		* @param uniformName Name of the uniform variable.
		* @return Integer value stored in the shader.
		*/
		int GetUniformInt(const std::string& uniformName) const override;

		/**
		* @brief Retrieves a float uniform value from the shader.
		* @param uniformName Name of the uniform variable.
		* @return Float value stored in the shader.
		*/
		float GetUniformFloat(const std::string& uniformName) const override;

		/**
		* @brief Retrieves a mat4 uniform value from the shader.
		* @param uniformName Name of the uniform variable.
		* @return 4x4 matrix value stored in the shader.
		*/
		glm::mat4 GetUniformMat4(const std::string& uniformName) const override;

		/**
		* @brief Retrieves a vec2 uniform value from the shader.
		* @param uniformName Name of the uniform variable.
		* @return 2D vector value stored in the shader.
		*/
		glm::vec2 GetUniformVec2(const std::string& uniformName) const override;

		/**
		* @brief Retrieves a vec3 uniform value from the shader.
		* @param uniformName Name of the uniform variable.
		* @return 3D vector value stored in the shader.
		*/
		glm::vec3 GetUniformVec3(const std::string& uniformName) const override;

		/**
		* @brief Returns the OpenGL shader program ID.
		* @return Shader program ID.
		*/
		unsigned int GetShaderID() override;

	private:
		unsigned int m_shaderID{ 0 };								/**< OpenGL shader program ID. */
		std::unordered_map<std::string, int> m_uniformLocations;	/**< Cached uniform locations for faster access. */
	};
} // namespace pwg

#endif // !SRC_GRAPHICS_SHADER_SHADER_H_
