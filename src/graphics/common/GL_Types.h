#ifndef SRC_GRAPHICS_COMMON_GL_TYPES_H_
#define SRC_GRAPHICS_COMMON_GL_TYPES_H_

#include <cstdint>

namespace pwg
{
	enum class TextureWrapMode : uint8_t
	{
		REPEAT,
		CLAMP_TO_EDGE,
		MIRRORED_REPEAT
	};

	enum class TextureFilterMode : uint8_t
	{
		LINEAR,
		NEAREST,
		MIPMAP_LINEAR_NEAREST,
		MIPMAP_LINEAR_LINEAR,
		MIPMAP_NEAREST_NEAREST,
		MIPMAP_NEAREST_LINEAR
	};

	enum class TextureFormats : uint8_t
	{
		R8,
		R32F,
		RGBA8,
		RGBA16F,
		RGBA32F
		
	};

	struct GLTextureFormats
	{
		int internalFormat;
		int format;
		int type;
	};

	constexpr unsigned int ToGL(TextureWrapMode mode);
	constexpr unsigned int ToGL(TextureFilterMode mode);
	constexpr GLTextureFormats ToGL(TextureFormats format);
}

#include "GL_Types.inl"

#endif // !SRC_GRAPHICS_COMMON_GL_TYPES_H_
