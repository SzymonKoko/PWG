#ifndef SRC_RESOURCES_RESOURCE_MANAGER_H
#define SRC_RESOURCES_RESOURCE_MANAGER_H

#include <memory>
#include "graphics/shader/shaderManager.h"
#include "graphics/texture/textureManager.h"
#include "graphics/mesh/meshManager.h"

namespace pwg
{
	class ResourceManager
	{
	public:
		ResourceManager();
		~ResourceManager() = default;

		//Shader

		ShaderManager& GetShaderManager() { return *m_shaderManager; }
		TextureManager& GetTextureManager() { return *m_textureManager; }
		MeshManager& GetMeshManager() { return *m_meshManager; }

		void UnloadAll();

		//Texture

	private:
		std::shared_ptr<ShaderManager> m_shaderManager;
		std::shared_ptr<TextureManager> m_textureManager;
		std::shared_ptr<MeshManager> m_meshManager;

	};
}
#endif // !SRC_RESOURCES_RESOURCE_MANAGER_H
