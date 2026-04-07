#ifndef SRC_GRAPHICS_TEXTURE_CUBE_MAP_TEXTURE_H_
#define SRC_GRAPHICS_TEXTURE_CUBE_MAP_TEXTURE_H_

#include <vector>
#include <string>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <iostream>
#include "core/logger/logger.h"

namespace pwg
{
	class CubeMapTexture
	{
	public:
		CubeMapTexture(std::vector<std::string> facesPaths);
		~CubeMapTexture() = default;

		void Bind(int slot);
		void Unbind();

		unsigned int GetCubeMapID();

	private:
		unsigned int m_cubeMapID{ 0 };

		unsigned char* m_image{ nullptr };							/**< Raw image data loaded from file. */
		int m_width{ 0 };											/**< Texture width in pixels. */
		int m_height{ 0 };											/**< Texture height in pixels. */
		int m_nrChannels{ 0 };										/**< Number of color channels. */
	};
}
#endif // !SRC_GRAPHICS_TEXTURE_CUBE_MAP_TEXTURE_H_
