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
	* @brief Class responsible for creating, managing, and dispatching OpenGL compute shaders.
	* Provides utility functions to bind textures, set uniforms, and perform compute operations.
	*/
	class ComputeShader : public IShader
	{
	public:

		/**
		* @brief Constructs a compute shader from a given file path.
		* Compiles and links the compute shader into a program.
		* @param computeFilePath Path to the compute shader source file.
		*/
		ComputeShader(const std::filesystem::path& computeFilePath);

		/**
		* @brief Destructor. Deletes the compute shader program.
		*/
		~ComputeShader();

		/**
		* @brief Reads shader code from a file.
		* @param shaderPath Path to the shader file.
		* @return Shader source code as a string.
		*/
		std::string ReadFromShaderFile(const std::filesystem::path& shaderPath) override;

		/**
		* @brief Preprocesses shader source, e.g., handling includes or macros.
		* @param shaderSource Raw shader code.
		* @param filePath Path to the original shader file (for relative includes).
		* @return Processed shader source code.
		*/
		std::string PreprocessShader(const std::string& shaderSource, const std::filesystem::path& filePath) override;

		/**
		* @brief Activates the compute shader program for dispatch.
		*/
		void ActivateShader() override;

		/**
		* @brief Deletes the compute shader program from GPU memory.
		*/
		void DeleteShader() override;

		/**
		* @brief Dispatches compute work groups.
		* @param x Number of work groups in X dimension.
		* @param y Number of work groups in Y dimension (default 1).
		* @param z Number of work groups in Z dimension (default 1).
		*/
		void Dispatch(const int x, const int y = 1, const int z = 1);

		/**
		* @brief Dispatches compute shader to process a texture of given size.
		* @param width Texture width.
		* @param height Texture height.
		*/
		void DispatchForTexture(const int width, const int height);

		/**
		* @brief Inserts a memory barrier to ensure proper ordering of GPU memory operations.
		* @param bits OpenGL memory barrier bits.
		*/
		void MemoryBarrier(const int bits);

		/**
		* @brief Binds a texture as an image for read/write operations in the compute shader.
		* @param textureUnit Texture unit to bind to.
		* @param textureID OpenGL texture ID.
		* @param access Access type (GL_READ_ONLY, GL_WRITE_ONLY, GL_READ_WRITE).
		* @param format OpenGL internal format of the image.
		*/
		void BindImage(const unsigned int textureUnit, const unsigned int textureID, const unsigned int access, const unsigned int format);

		/**
		* @brief Binds a texture to a sampler uniform in the compute shader.
		* @param textureUnit Texture unit to bind to.
		* @param textureID OpenGL texture ID.
		*/
		void BindTextureSampler(const unsigned int textureUnit, const unsigned int textureID);

		/**
		* @brief Sets a boolean uniform.
		* @param name Uniform variable name.
		* @param value Boolean value to assign.
		*/
		void SetUniformBool(const std::string& name, bool value) const override;

		/**
		* @brief Sets an integer uniform.
		* @param name Uniform variable name.
		* @param value Integer value to assign.
		*/
		void SetUniformInt(const std::string& name, int value) const override;

		/**
		* @brief Sets a float uniform.
		* @param name Uniform variable name.
		* @param value Float value to assign.
		*/
		void SetUniformFloat(const std::string& name, float value) const override;

		/**
		* @brief Sets a mat4 uniform.
		* @param name Uniform variable name.
		* @param value 4x4 matrix to assign.
		*/
		void SetUniformMat4(const std::string& name, glm::mat4 value) const override;

		/**
		* @brief Sets a vec2 uniform.
		* @param name Uniform variable name.
		* @param value 2D vector to assign.
		*/
		void SetUniformVec2(const std::string& name, glm::vec2 value) const override;

		/**
		* @brief Sets a vec3 uniform.
		* @param name Uniform variable name.
		* @param value 3D vector to assign.
		*/
		void SetUniformVec3(const std::string& name, glm::vec3 value) const override;

		/**
		* @brief Retrieves the OpenGL uniform location.
		* @param uniformName Name of the uniform variable.
		* @return Location ID of the uniform.
		*/
		int GetUniformLocation(const std::string& uniformName) override;

		/**
		* @brief Retrieves an integer uniform value from the shader.
		* @param uniformName Uniform variable name.
		* @return Integer value stored in the shader.
		*/
		int GetUniformInt(const std::string& uniformName) const override;

		/**
		* @brief Retrieves a float uniform value from the shader.
		* @param uniformName Uniform variable name.
		* @return Float value stored in the shader.
		*/
		float GetUniformFloat(const std::string& uniformName) const override;

		/**
		* @brief Retrieves a mat4 uniform value from the shader.
		* @param uniformName Uniform variable name.
		* @return 4x4 matrix value stored in the shader.
		*/
		glm::mat4 GetUniformMat4(const std::string& uniformName) const override;

		/**
		* @brief Retrieves a vec2 uniform value from the shader.
		* @param uniformName Uniform variable name.
		* @return 2D vector value stored in the shader.
		*/
		glm::vec2 GetUniformVec2(const std::string& uniformName) const override;

		/**
		* @brief Retrieves a vec3 uniform value from the shader.
		* @param uniformName Uniform variable name.
		* @return 3D vector value stored in the shader.
		*/
		glm::vec3 GetUniformVec3(const std::string& uniformName) const override;

		/**
		* @brief Returns the OpenGL shader program ID.
		* @return Shader program ID.
		*/
		unsigned int GetShaderID() override;

	private:
		unsigned int m_shaderID{ 0 };								/**< OpenGL shader program ID */
		glm::uvec3 m_localGroupSize;								/**< Local work group size specified in the compute shader */
		std::unordered_map<std::string, int> m_uniformLocations;	/**< Cached uniform locations */
	};
} // namespace pwg

#endif // !SRC_GRAPHICS_SHADER_COMPUTE_SHADER_H_
