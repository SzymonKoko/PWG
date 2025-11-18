#ifndef SRC_GRAPHICS_SHADER_SHADER_MANAGER_H
#define SRC_GRAPHICS_SHADER_SHADER_MANAGER_H

#include "shader.h"
#include <string>
#include <memory>
#include <unordered_map>

namespace pwg
{
	/**
	 * @brief Class responsible for managing multiple Shader instances.
	 * Allows loading, retrieving, and unloading shaders by ID.
	 */
	class ShaderManager
	{
	public:

		/**
		 * @brief Default constructor. Initializes an empty shader manager.
		 */
		ShaderManager() = default;

		/**
		 * @brief Destructor. Cleans up all loaded shaders.
		 */
		~ShaderManager() = default;

		/**
		 * @brief Loads a shader from vertex and fragment shader files and stores it by ID.
		 * @param shaderID Unique identifier for the shader.
		 * @param vertexPath Path to the vertex shader file.
		 * @param fragmentPath Path to the fragment shader file.
		 */
		void Load(const std::string& shaderID, const std::string& vertexPath, const std::string& fragmentPath);

		/**
		 * @brief Loads a shader from compute shader files and stores it by ID.
		 * @param shaderID Unique identifier for the shader.
		 * @param computePath Path to the compute shader file.
		 */
		void Load(const std::string& shaderID, const std::string& computePath);

		/**
		 * @brief Unloads and deletes a shader by its ID.
		 * @param shaderID ID of the shader to remove.
		 */
		void Unload(std::string& shaderID);

		/**
		 * @brief Unloads all shaders currently stored in the manager.
		 */
		void UnloadAll();

		/**
		 * @brief Retrieves a shader by its ID.
		 * @param shaderID ID of the shader to retrieve.
		 * @return Shared pointer to the requested Shader instance.
		 */
		std::shared_ptr<Shader> GetShader(const std::string& shaderID);

	private:
		std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaders; /**< Container storing all loaded shaders. */
	};
} // namespace pwg

#endif // !SRC_GRAPHICS_SHADER_SHADER_MANAGER_H
