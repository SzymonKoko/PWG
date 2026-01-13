#ifndef SRC_GRAPHICS_TEXTURE_TEXTURE_H_
#define SRC_GRAPHICS_TEXTURE_TEXTURE_H_

#include <iostream>
#include <fstream>
#include <string>
#include <stb_image.h>
#include "graphics/common/GL_Types.h"

#include "core/logger/logger.h"

namespace pwg
{
	/**
	* @brief Enum representing different types of textures.
	*/
	enum class TextureType
	{
		Texture2D,      /**< Standard 2D texture. */
		CubeMap,        /**< Cubemap texture for environment mapping. */
		NoiseTexture    /**< Procedural noise texture. */
	};

	/**
	* @brief Class responsible for creating, managing, and using OpenGL textures.
	* Supports loading from image files, creating empty textures, and configuring filtering/wrapping.
	*/
	class Texture
	{
	public:

		/**
		* @brief Default constructor. Creates an empty texture object.
		*/
		Texture();

		/**
		* @brief Loads a texture from an image file.
		* @param imagePath Path to the image file.
		*/
		Texture(const std::string& imagePath);

		/**
		* @brief Creates an empty texture with specified dimensions and format.
		* @param width Texture width in pixels.
		* @param height Texture height in pixels.
		* @param format OpenGL texture format specification.
		*/
		Texture(int width, int height, GLTextureFormats format);

		/**
		* @brief Destructor. Frees image memory and deletes the OpenGL texture.
		*/
		~Texture();

		/**
		* @brief Binds the texture to a specific texture slot for rendering.
		* @param slot Texture slot to bind to.
		*/
		void Bind(int slot);

		/**
		* @brief Unbinds the texture, setting the active texture slot to 0.
		*/
		void Unbind();

		/**
		* @brief Sets the texture wrapping mode for S and T coordinates.
		* @param wrapS Wrapping mode for S axis.
		* @param wrapT Wrapping mode for T axis.
		*/
		void SetTextureWrapping(int wrapS, int wrapT);

		/**
		* @brief Sets the texture filtering parameters.
		* @param minFilter Minification filter.
		* @param magFilter Magnification filter.
		*/
		void SetTextureFiltering(int minFilter, int magFilter);

		/**
		* @brief Updates the texture data with new pixel information.
		* @param data Pointer to raw image data.
		* @param width Texture width in pixels.
		* @param height Texture height in pixels.
		* @param nrChannels Number of color channels (e.g., RGB=3, RGBA=4).
		*/
		void UpdateData(unsigned char* data, int width, int height, int nrChannels);

		/**
		* @brief Returns the OpenGL texture ID.
		* @return Texture ID.
		*/
		unsigned int GetTextureID();

		/**
		* @brief Returns the width of the texture.
		* @return Width in pixels.
		*/
		int GetWidth();

		/**
		* @brief Returns the height of the texture.
		* @return Height in pixels.
		*/
		int GetHeight();

	private:
		unsigned char* m_image{ nullptr };							/**< Raw image data loaded from file. */
		int m_width{ 0 };											/**< Texture width in pixels. */
		int m_height{ 0 };											/**< Texture height in pixels. */
		int m_nrChannels{ 0 };										/**< Number of color channels. */

		unsigned int m_textureID{ 0 };								/**< OpenGL texture ID. */

		int m_slot{ 0 };											/**< Texture slot binding index. */

		int m_wrapS;												/**< Wrapping mode for S coordinate. */
		int m_wrapT;												/**< Wrapping mode for T coordinate. */

		int m_minFilter;											/**< Minification filter mode. */
		int m_magFilter;											/**< Magnification filter mode. */

		bool m_hasMipmap{ true };									/**< Whether mipmaps are generated for the texture. */

		TextureType m_textureType{ TextureType::Texture2D };		/**< Type of texture (2D, cubemap, noise). */
		GLTextureFormats m_format;									/**< OpenGL internal format, format, and type. */
	};
} // namespace pwg

#endif // !SRC_GRAPHICS_TEXTURE_TEXTURE_H_
