#include "resourceManager.h"

pwg::ResourceManager::ResourceManager()
{
	m_shaderManager = std::make_shared<ShaderManager>();
	m_textureManager = std::make_shared<TextureManager>();
	m_meshManager = std::make_shared<MeshManager>();

	PWG_INFO("Resource manager initialized");
}

void pwg::ResourceManager::UnloadAll()
{
	m_shaderManager->UnloadAll();
	m_textureManager->UnloadAll();
	
}
