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
	class TextureArray
	{
	public:
		TextureArray(int width, int height, int numberOfLayers, GLTextureFormats format);
		~TextureArray() = default;

		void SetLayerData(int layer, unsigned char* data);
		void SubmitTextureArray();
		void LoadTexturesIntoArray(std::vector<std::string> texturePaths);

		void Bind(int slot);

	private:

		unsigned int m_textureArrayID;
		int m_slot{ 0 };

		int m_width{ 0 };
		int m_height{ 0 };
		int m_numberOfLayers{ 0 };

		GLTextureFormats m_format;

		int m_wrapS;
		int m_wrapT;

		int m_minFilter;
		int m_magFilter;

	};
	
} // namespace pwg

#endif // !SRC_GRAPHICS_TEXTURE_TEXTURE_ARRAY_H_


//Filtering
//Wrapping
//Slot
//Texture type

