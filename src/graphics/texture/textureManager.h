#ifndef SRC_GRAPHICS_TEXTURE_TEXTURE_MANAGER_H
#define SRC_GRAPHICS_TEXTURE_TEXTURE_MANAGER_H

#include "texture.h"
#include <string>
#include <memory>
#include <unordered_map>

namespace pwg
{
	class TextureManager
	{
	public:
		TextureManager() = default;
		~TextureManager() = default;

		void Load(const std::string& textureID, const std::string& texturePath);
		void Unload(std::string& textureID);
		void UnloadAll();
		std::shared_ptr<pwg::Texture> GetTexture(const std::string& textureID);
	private:
		std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;

	};
}
#endif // !SRC_GRAPHICS_TEXTURE_TEXTURE_MANAGER_H
