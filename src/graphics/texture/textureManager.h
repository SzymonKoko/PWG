#ifndef SRC_GRAPHICS_TEXTURE_TEXTURE_MANAGER_H
#define SRC_GRAPHICS_TEXTURE_TEXTURE_MANAGER_H

#include "texture.h"
#include "textureArray.h"
#include "cubeMapTexture.h"
#include <string>
#include <memory>
#include <unordered_map>

namespace pwg
{
	/**
	* @brief Class responsible for managing multiple Texture and TextureArray instances.
	* Provides functionality to load, retrieve, and unload textures by unique ID.
	*/
	class TextureManager
	{
	public:

		/**
		* @brief Default constructor. Initializes an empty texture manager.
		*/
		TextureManager() = default;

		/**
		* @brief Destructor. Cleans up all loaded textures and texture arrays.
		*/
		~TextureManager() = default;

		/**
		* @brief Loads a 2D texture from a file and stores it with the specified ID.
		* @param textureID Unique identifier for the texture.
		* @param texturePath Path to the texture image file.
		*/
		void Load(const std::string& textureID, const std::string& texturePath);

		/**
		* @brief Loads a texture array from multiple image files and stores it with the given name.
		* @param name Unique identifier for the texture array.
		* @param texturePaths Vector of file paths for the individual layers.
		*/
		void LoadTextureArray(const std::string& name, std::vector<std::string> texturePaths, TextureFormats format);

		void LoadCubeMapTexture(const std::string& name, std::vector<std::string> cubemapPaths);

		/**
		* @brief Unloads a texture by its ID and deletes it from GPU memory.
		* @param textureID ID of the texture to remove.
		*/
		void Unload(std::string& textureID);

		/**
		* @brief Unloads all textures and texture arrays currently stored in the manager.
		*/
		void UnloadAll();

		/**
		* @brief Retrieves a 2D texture by its ID.
		* @param textureID ID of the texture to retrieve.
		* @return Shared pointer to the requested Texture instance.
		*/
		std::shared_ptr<pwg::Texture> GetTexture(const std::string& textureID);

		/**
		* @brief Retrieves a texture array by its ID.
		* @param textureArrayID ID of the texture array to retrieve.
		* @return Shared pointer to the requested TextureArray instance.
		*/
		std::shared_ptr<TextureArray> GetTextureArray(const std::string& textureArrayID);

		std::shared_ptr<CubeMapTexture> GetCubeMapTexture(const std::string& cubemapTextureID);

	private:
		std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;			/**< Container storing all loaded 2D textures. */
		std::unordered_map<std::string, std::shared_ptr<TextureArray>> m_textureArrays; /**< Container storing all loaded texture arrays. */
		std::unordered_map<std::string, std::shared_ptr<CubeMapTexture>> m_cubeMapTextures; /**< Container storing all loaded texture arrays. */
	};
} // namespace pwg

#endif // !SRC_GRAPHICS_TEXTURE_TEXTURE_MANAGER_H
