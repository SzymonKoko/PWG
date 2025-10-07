#include "textureManager.h"

void pwg::TextureManager::Load(const std::string& textureID, const std::string& texturePath)
{
    if (m_textures.contains(textureID))
    {
        return;
    }

    auto texture = std::make_shared<Texture>(texturePath);
    m_textures[textureID] = texture;

    //Czy w przypadku framebuffer tex zostawiæ to w Texture czy daæ drugi konstruktor
}

void pwg::TextureManager::Unload(std::string& textureID)
{
    m_textures.erase(textureID);
}

void pwg::TextureManager::UnloadAll()
{
    m_textures.clear();
}

std::shared_ptr<pwg::Texture> pwg::TextureManager::GetTexture(const std::string& textureID)
{
    if (!m_textures.contains(textureID))
    {
        return nullptr;
    }
    return m_textures[textureID];
}
