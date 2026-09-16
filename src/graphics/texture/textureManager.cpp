#include "textureManager.h"
#include <algorithm>

void pwg::TextureManager::Load(const std::string& textureID, const std::string& texturePath, TextureType type)
{
    if (m_textures.contains(textureID))
    {
        return;
    }

    auto texture = std::make_shared<Texture>(texturePath, type);
    m_textures[textureID] = texture;
    
    PWG_DEBUG("Texture has been loaded ({0}, id={1}, {2}x{3})", textureID, texture->GetTextureID(), texture->GetWidth(), texture->GetHeight());
}

bool pwg::TextureManager::LoadTextureArray(const std::string& name, const std::vector<std::string>& texturePaths, TextureType type)
{
    if (m_textureArrays.contains(name))
    {
		return true;
    }
	std::string error;
	auto textureArray = CreateTextureArray(texturePaths, type, {}, &error);
	if (!textureArray)
	{
		PWG_ERROR("Cannot load texture array '{}': {}", name, error);
		return false;
	}
	m_textureArrays[name] = textureArray;
	return true;
}

bool pwg::TextureManager::ReloadTextureArray(const std::string& name, const std::vector<std::string>& texturePaths, TextureType type)
{
	std::string error;
	auto textureArray = CreateTextureArray(texturePaths, type, {}, &error);
	if (!textureArray)
	{
		PWG_ERROR("Cannot reload texture array '{}': {}", name, error);
		return false;
	}
	m_textureArrays[name] = textureArray;
	return true;
}

std::shared_ptr<pwg::TextureArray> pwg::TextureManager::CreateTextureArray(
	const std::vector<std::string>& texturePaths,
	TextureType type,
	const std::vector<unsigned char>& fallbackValues,
	std::string* error)
{
	if (texturePaths.empty())
	{
		if (error) *error = "No texture paths";
		return nullptr;
	}
	auto first = std::find_if(texturePaths.begin(), texturePaths.end(), [](const std::string& path) { return !path.empty(); });
	if (first == texturePaths.end())
	{
		if (error) *error = "No source texture available for dimensions";
		return nullptr;
	}
	int width = 0;
	int height = 0;
	int channels = 0;
	if (!stbi_info(first->c_str(), &width, &height, &channels))
	{
		if (error) *error = "Failed to read " + *first;
		return nullptr;
	}
	auto textureArray = std::make_shared<TextureArray>(width, height, static_cast<int>(texturePaths.size()), type);
	if (!textureArray->LoadTexturesIntoArray(texturePaths, fallbackValues, error)) return nullptr;
	return textureArray;
}

void pwg::TextureManager::SetTextureArray(const std::string& name, std::shared_ptr<TextureArray> textureArray)
{
	m_textureArrays[name] = std::move(textureArray);
}

void pwg::TextureManager::LoadCubeMapTexture(const std::string& name, std::vector<std::string> cubemapPaths, TextureType type)
{
    if (m_cubeMapTextures.contains(name))
    {
        return;
    }

    auto cubeMapTexture = std::make_shared<CubeMapTexture>(cubemapPaths, type);

    m_cubeMapTextures[name] = cubeMapTexture;
}

void pwg::TextureManager::ReloadCubeMapTexture(const std::string& name, std::vector<std::string> cubemapPaths, TextureType type)
{
    if (cubemapPaths.size() != 6)
    {
        PWG_ERROR("Cannot reload cubemap '{}': expected 6 faces, got {}", name, cubemapPaths.size());
        return;
    }

    auto cubeMapTexture = std::make_shared<CubeMapTexture>(cubemapPaths, type);
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
