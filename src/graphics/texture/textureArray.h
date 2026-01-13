#ifndef SRC_GRAPHICS_TEXTURE_TEXTURE_ARRAY_H_
#define SRC_GRAPHICS_TEXTURE_TEXTURE_ARRAY_H_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stb_image.h>
#include "graphics/common/GL_Types.h"

#include "core/logger/logger.h"

namespace pwg
{
	/**
	* @brief Class responsible for creating and managing OpenGL texture arrays.
	* Allows setting individual layer data, binding, and configuring filtering/wrapping modes.
	*/
	class TextureArray
	{
	public:
		/**
		* @brief Constructs a texture array with specified dimensions, layers, and format.
		* @param width Width of each layer in pixels.
		* @param height Height of each layer in pixels.
		* @param numberOfLayers Total number of layers in the texture array.
		* @param format OpenGL internal texture format specification.
		*/
		TextureArray(int width, int height, int numberOfLayers, GLTextureFormats format);

		/**
		* @brief Destructor. Deletes the OpenGL texture array.
		*/
		~TextureArray() = default;

		/**
		* @brief Sets the pixel data for a specific layer in the texture array.
		* @param layer Layer index to update.
		* @param data Pointer to raw image data for the layer.
		*/
		void SetLayerData(int layer, unsigned char* data);

		/**
		* @brief Submits the texture array to GPU memory after all layer data has been set.
		*/
		void SubmitTextureArray();

		/**
		* @brief Loads multiple textures from files into the texture array.
		* @param texturePaths Vector of file paths to the textures to load.
		*/
		void LoadTexturesIntoArray(std::vector<std::string> texturePaths);

		/**
		* @brief Binds the texture array to a specified texture slot.
		* @param slot Texture slot to bind to.
		*/
		void Bind(int slot);

	private:
		unsigned int m_textureArrayID; /**< OpenGL texture array ID. */
		int m_slot{ 0 };               /**< Texture slot binding index. */

		int m_width{ 0 };              /**< Width of each layer in pixels. */
		int m_height{ 0 };             /**< Height of each layer in pixels. */
		int m_numberOfLayers{ 0 };     /**< Total number of layers in the array. */

		GLTextureFormats m_format;     /**< Format specification for all layers. */

		int m_wrapS;                   /**< Wrapping mode for S coordinate. */
		int m_wrapT;                   /**< Wrapping mode for T coordinate. */

		int m_minFilter;               /**< Minification filter mode. */
		int m_magFilter;               /**< Magnification filter mode. */
	};

} // namespace pwg

#endif // !SRC_GRAPHICS_TEXTURE_TEXTURE_ARRAY_H_
