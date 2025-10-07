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
	 * @brief Creating, activating and deleteing textures
	 */
	class Texture
	{
	public:
		Texture();
		Texture(const std::string& imagePath);
		Texture(int width, int height);
		~Texture();


		/**
		* @brief Binding current texture
		*/
		void Bind();


		/**
		* @brief Unbinding current texture
		*/
		void Unbind();

		unsigned int GetTextureID() { return m_textureID; }



	private:
		unsigned char* m_image{ nullptr };
		int m_width{ 0 };
		int m_height{ 0 };
		int m_nrChannels{ 0 };
		unsigned int m_textureID{ 0 };

	};
}
#endif // !SRC_GRAPHICS_TEXTURE_TEXTURE_H_
