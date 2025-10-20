#ifndef SRC_GRAPHICS_TEXTURE_TEXTURE_H_
#define SRC_GRAPHICS_TEXTURE_TEXTURE_H_

#include <iostream>
#include <fstream>
#include <string>
#include <stb_image.h>

#include "core/logger/logger.h"

namespace pwg
{
	/**
	 * @brief Class responsible for creating, binding, and deleting textures.
	 * Supports loading from image files or creating empty textures with specified dimensions.
	 */
	class Texture
	{
	public:

		/**
		 * @brief Default constructor. Creates an empty texture.
		 */
		Texture();

		/**
		 * @brief Constructs a texture from an image file.
		 * @param imagePath Path to the image file to load.
		 */
		Texture(const std::string& imagePath);

		/**
		 * @brief Constructs an empty texture with specified width and height.
		 * @param width Width of the texture in pixels.
		 * @param height Height of the texture in pixels.
		 */
		Texture(int width, int height);

		/**
		 * @brief Destructor. Frees image memory and deletes the OpenGL texture.
		 */
		~Texture();

		/**
		 * @brief Binds the texture as the current active texture in OpenGL.
		 */
		void Bind();

		/**
		 * @brief Unbinds the texture, setting the active texture to 0.
		 */
		void Unbind();

		/**
		 * @brief Getter for OpenGL texture ID.
		 * @return OpenGL texture ID.
		 */
		unsigned int GetTextureID() { return m_textureID; }

		/**
		 * @brief Getter for texture width.
		 * @return Width of the texture in pixels.
		 */
		int GetWidth() { return m_width; }

		/**
		 * @brief Getter for texture height.
		 * @return Height of the texture in pixels.
		 */
		int GetHeight() { return m_height; }

	private:
		unsigned char* m_image{ nullptr }; /**< Raw image data loaded from file. */
		int m_width{ 0 };                  /**< Texture width in pixels. */
		int m_height{ 0 };                 /**< Texture height in pixels. */
		int m_nrChannels{ 0 };             /**< Number of channels in the texture (RGB, RGBA, etc.). */
		unsigned int m_textureID{ 0 };     /**< OpenGL texture ID. */
	};
} // namespace pwg

#endif // !SRC_GRAPHICS_TEXTURE_TEXTURE_H_
