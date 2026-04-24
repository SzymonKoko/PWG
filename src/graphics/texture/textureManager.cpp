#include "textureManager.h"

void pwg::TextureManager::Load(const std::string& textureID, const std::string& texturePath)
{
    if (m_textures.contains(textureID))
    {
        return;
    }

    auto texture = std::make_shared<Texture>(texturePath);
    m_textures[textureID] = texture;
    
    PWG_DEBUG("Texture has been loaded ({0}, id={1}, {2}x{3})", textureID, texture->GetTextureID(), texture->GetWidth(), texture->GetHeight());
}

void pwg::TextureManager::LoadTextureArray(const std::string& name, std::vector<std::string> texturePaths, TextureFormats format)
{
    if (m_textureArrays.contains(name))
    {
        return;
    }

    auto textureArray = std::make_shared<TextureArray>(1024, 1024, (int)texturePaths.size(), ToGL(format));

    textureArray->LoadTexturesIntoArray(texturePaths);

    m_textureArrays[name] = textureArray;

}

void pwg::TextureManager::LoadCubeMapTexture(const std::string& name, std::vector<std::string> cubemapPaths)
{
    if (m_cubeMapTextures.contains(name))
    {
        return;
    }

    auto cubeMapTexture = std::make_shared<CubeMapTexture>(cubemapPaths);

    m_cubeMapTextures[name] = cubeMapTexture;
}

void pwg::TextureManager::Unload(std::string& textureID)
{
    m_textures.erase(textureID);
    PWG_DEBUG("Texture has been unloaded ({0})", textureID);
}

void pwg::TextureManager::UnloadAll()
{
    m_textures.clear();
    PWG_DEBUG("All textures have been unloaded");
}

std::shared_ptr<pwg::Texture> pwg::TextureManager::GetTexture(const std::string& textureID)
{
    if (!m_textures.contains(textureID))
    {
        PWG_ERROR("Texture {0} does not exits!\n", textureID);
        return nullptr;
    }
    return m_textures[textureID];
}

std::shared_ptr<pwg::TextureArray> pwg::TextureManager::GetTextureArray(const std::string& textureArrayID)
{
    return m_textureArrays[textureArrayID];
}

std::shared_ptr<pwg::CubeMapTexture> pwg::TextureManager::GetCubeMapTexture(const std::string& cubemapTextureID)
{
    return m_cubeMapTextures[cubemapTextureID];
}
