#ifndef SRC_GRAPHICS_TEXTURE_TEXTURE_MANAGER_H
#define SRC_GRAPHICS_TEXTURE_TEXTURE_MANAGER_H

#include "texture.h"
#include "textureArray.h"
#include <string>
#include <memory>
#include <unordered_map>

namespace pwg
{
	/**
	 * @brief Class responsible for managing multiple Texture instances.
	 * Provides functionality to load, retrieve, and unload textures by ID.
	 */
	class TextureManager
	{
	public:

		/**
		 * @brief Default constructor. Initializes an empty texture manager.
		 */
		TextureManager() = default;

		/**
		 * @brief Destructor. Cleans up all loaded textures.
		 */
		~TextureManager() = default;

		/**
		 * @brief Loads a texture from a file and stores it with the given ID.
		 * @param textureID Unique identifier for the texture.
		 * @param texturePath Path to the texture image file.
		 */
		void Load(const std::string& textureID, const std::string& texturePath);

		void LoadTextureArray(const std::string& name, std::vector<std::string> texturePaths);

		/**
		 * @brief Unloads a texture by its ID and deletes it from GPU memory.
		 * @param textureID ID of the texture to remove.
		 */
		void Unload(std::string& textureID);

		/**
		 * @brief Unloads all textures currently stored in the manager.
		 */
		void UnloadAll();

		/**
		 * @brief Retrieves a texture by its ID.
		 * @param textureID ID of the texture to retrieve.
		 * @return Shared pointer to the requested Texture instance.
		 */
		std::shared_ptr<pwg::Texture> GetTexture(const std::string& textureID);
		std::shared_ptr<TextureArray> GetTextureArray(const std::string& textureArrayID);

	private:
		std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures; /**< Container storing all loaded textures. */
		std::unordered_map<std::string, std::shared_ptr<TextureArray>> m_textureArrays;
	};
} // namespace pwg

#endif // !SRC_GRAPHICS_TEXTURE_TEXTURE_MANAGER_H
