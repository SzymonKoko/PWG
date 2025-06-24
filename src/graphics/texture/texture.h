#ifndef SRC_GRAPHICS_TEXTURE_TEXTURE_H_
#define SRC_GRAPHICS_TEXTURE_TEXTURE_H_


#include <glad.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stb_image.h>

#include "core/logger/logger.h"

namespace pwg
{
	/**
	 * @brief Creating, activating and deleteing textures
	 */
	class Texture
	{
	public:
		Texture();
		~Texture();


		/**
		* @brief Binding current texture
		*/
		void Bind();


		/**
		* @brief Unbinding current texture
		*/
		void Unbind();

		/**
		* @brief Loading image file to imageData variable
		* @param imagePath Path to the image
		* @param width Width of an image
		* @param height Height of an image
		* @param nrChannels Number of color channels 
		*/
		void LoadFromFile(const std::string& imagePath);

	private:
		unsigned char* m_image;
		int m_width{ 0 };
		int m_height{ 0 };
		int m_nrChannels{ 0 };
		GLuint m_textureID{ 0 };

	};
}
#endif // !SRC_GRAPHICS_TEXTURE_TEXTURE_H_
