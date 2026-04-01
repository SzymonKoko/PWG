#ifndef SRC_RESOURCES_RESOURCE_MANAGER_H
#define SRC_RESOURCES_RESOURCE_MANAGER_H

#include <memory>
#include "graphics/shader/shaderManager.h"
#include "graphics/texture/textureManager.h"
#include "graphics/mesh/meshManager.h"
#include "graphics/material/materialManager.h"

namespace pwg
{
	/**
	 * @brief Class responsible for managing all game resources.
	 * Provides access to ShaderManager, TextureManager, and MeshManager.
	 * Allows centralized loading, retrieval, and unloading of resources.
	 */
	class ResourceManager
	{
	public:

		/**
		 * @brief Constructs the resource manager and initializes all sub-managers.
		 */
		ResourceManager();

		/**
		 * @brief Destructor. Cleans up all resources via sub-managers.
		 */
		~ResourceManager() = default;

		/**
		 * @brief Returns reference to the ShaderManager.
		 * @return Reference to ShaderManager instance.
		 */
		std::shared_ptr<ShaderManager> GetShaderManager() { return m_shaderManager; }

		/**
		 * @brief Returns reference to the TextureManager.
		 * @return Reference to TextureManager instance.
		 */
		std::shared_ptr<TextureManager> GetTextureManager() { return m_textureManager; }

		/**
		 * @brief Returns reference to the MeshManager.
		 * @return Reference to MeshManager instance.
		 */
		std::shared_ptr<MeshManager> GetMeshManager() { return m_meshManager; }

		/**
		 * @brief Returns reference to the MaterialManager.
		 * @return Reference to MaterialManager instance.
		 */
		std::shared_ptr<MaterialManager> GetMaterialManager() { return m_materialManager; }

		/**
		 * @brief Unloads all resources from all managers.
		 */
		void UnloadAll();

	private:
		std::shared_ptr<ShaderManager> m_shaderManager;			/**< Manages all shaders. */
		std::shared_ptr<TextureManager> m_textureManager;		/**< Manages all textures. */
		std::shared_ptr<MeshManager> m_meshManager;				/**< Manages all meshes. */
		std::shared_ptr<MaterialManager> m_materialManager;     /**< Manages all materials. */
	};
} // namespace pwg

#endif // !SRC_RESOURCES_RESOURCE_MANAGER_H
