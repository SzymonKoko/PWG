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

	constexpr unsigned int ToGL(TextureWrapMode mode);
	constexpr unsigned int ToGL(TextureFilterMode mode);
}

#include "GL_Types.inl"

#endif // !SRC_GRAPHICS_COMMON_GL_TYPES_H_
