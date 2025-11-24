#ifndef SRC_GRAPHICS_SHADER_COMPUTE_SHADER_H_
#define SRC_GRAPHICS_SHADER_COMPUTE_SHADER_H_

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
	 * @brief Class responsible for creating, activating, and deleting OpenGL compute shaders.
	 * Provides convenient methods to set shader uniforms of various types.
	 */
	class ComputeShader : public IShader
	{
	public:

		/**
		 * @brief Constructs a compute shader program from compute shader files.
		 * Compiles and links shaders into a single program.
		 * @param computeFilePath Path to the compute shader file.
		 */
		ComputeShader(const std::string& computeFilePath);

		/**
		 * @brief Constructs a compute shader program from compute shader files. 
		 * Attaches another shader file to compute shader.
		 * Compiles and links shaders into a single program.
		 * @param computeFilePath Path to the compute shader file.
		 * @param shaderToAttachFilePath Path to the shader that have to be attached to compute shader.
		 */
		ComputeShader(const std::string& computeFilePath, const std::string& shaderToAttachFilePath);

		/**
		 * @brief Destructor. Deletes the compute shader program.
		 */
		~ComputeShader();

		/**
		 * @brief Reads shader code from a file.
		 * @param shaderPath Path to the shader file.
		 * @return Shader source code as a string.
		 */
		std::string ReadFromShaderFile(const std::string& shaderPath) override;

		/**
		 * @brief Activates the shader program for rendering.
		 */
		void ActivateShader() override;

		/**
		 * @brief Deletes the shader program from GPU memory.
		 */
		void DeleteShader() override;

		/**
		 * @brief Dispatches compute shader and sets quantity of work groups.
		 * @param x Number of work groups in X dimension.
		 * @param y Number of work groups in Y dimension. Default 1.
		 * @param z Number of work groups in Z dimension. Default 1.
		 */
		void Dispatch(const int x, const int y = 1, const int z = 1);

		void DispatchForTexture(const int width, const int height);

		void MemoryBarrier(const int bits);

		void BindImage(const unsigned int textureUnit, const unsigned int textureID, const unsigned int access, const unsigned int format);

		void BindTextureSampler(const unsigned int textureUnit, const unsigned int textureID);

		/**
		 * @brief Sets a boolean uniform in the shader.
		 * @param name Name of the uniform variable in the shader.
		 * @param value Boolean value to set.
		 */
		void SetUniformBool(const std::string& name, bool value) const override;

		/**
		 * @brief Sets an integer uniform in the shader.
		 * @param name Name of the uniform variable in the shader.
		 * @param value Integer value to set.
		 */
		void SetUniformInt(const std::string& name, int value) const override;

		/**
		 * @brief Sets a float uniform in the shader.
		 * @param name Name of the uniform variable in the shader.
		 * @param value Float value to set.
		 */
		void SetUniformFloat(const std::string& name, float value) const override;

		/**
		 * @brief Sets a mat4 uniform in the shader.
		 * @param name Name of the uniform variable in the shader.
		 * @param value 4x4 matrix value to set.
		 */
		void SetUniformMat4(const std::string& name, glm::mat4 value) const override;

		/**
		 * @brief Sets a vec2 uniform in the shader.
		 * @param name Name of the uniform variable in the shader.
		 * @param value 2D vector value to set.
		 */
		void SetUniformVec2(const std::string& name, glm::vec2 value) const override;

		/**
		 * @brief Sets a vec3 uniform in the shader.
		 * @param name Name of the uniform variable in the shader.
		 * @param value 3D vector value to set.
		 */
		void SetUniformVec3(const std::string& name, glm::vec3 value) const override;

		/**
		 * @brief Returns the OpenGL uniform location ID.
		 * @param uniformName Name of the uniform.
		 * @return Uniform location ID.
		 */
		int GetUniformLocation(const std::string& uniformName) override;

		/**
		 * @brief Returns the OpenGL uniform integer value.
		 * @param uniformName Name of the uniform.
		 * @return Uniform integer value.
		 */
		int GetUniformInt(const std::string& uniformName) const override;

		/**
		 * @brief Returns the OpenGL uniform float value.
		 * @param uniformName Name of the uniform.
		 * @return Uniform float value.
		 */
		float GetUniformFloat(const std::string& uniformName) const override;

		/**
		 * @brief Returns the OpenGL uniform mat4 value.
		 * @param uniformName Name of the uniform.
		 * @return Uniform mat4 value.
		 */
		glm::mat4 GetUniformMat4(const std::string& uniformName) const override;

		/**
		 * @brief Returns the OpenGL uniform vec2 value.
		 * @param uniformName Name of the uniform.
		 * @return Uniform vec2 value.
		 */
		glm::vec2 GetUniformVec2(const std::string& uniformName) const override;

		/**
		 * @brief Returns the OpenGL uniform vec3 value.
		 * @param uniformName Name of the uniform.
		 * @return Uniform vec3 value.
		 */
		glm::vec3 GetUniformVec3(const std::string& uniformName) const override;

		/**
		 * @brief Returns the OpenGL shader program ID.
		 * @return Shader program ID.
		 */
		unsigned int GetShaderID() override;

	private:
		unsigned int m_shaderID{ 0 }; /**< OpenGL shader program ID. */
		glm::uvec3 m_localGroupSize;
		std::unordered_map<std::string, int> m_uniformLocations;
	};
} // namespace pwg

#endif // !SRC_GRAPHICS_SHADER_COMPUTE_SHADER_H_
