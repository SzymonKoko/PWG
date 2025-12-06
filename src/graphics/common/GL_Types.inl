#ifndef SRC_GRAPHICS_COMMON_GL_TYPES_INL_
#define SRC_GRAPHICS_COMMON_GL_TYPES_INL_

#include <glad/glad.h>
#include "GL_Types.h"

namespace pwg
{
	constexpr unsigned int ToGL(TextureWrapMode mode)
	{
		switch (mode)
		{
			case TextureWrapMode::REPEAT:				return GL_REPEAT;
			case TextureWrapMode::CLAMP_TO_EDGE:		return GL_CLAMP_TO_EDGE;
			case TextureWrapMode::MIRRORED_REPEAT:		return GL_MIRRORED_REPEAT;
		}
		return GL_REPEAT;
	}

	constexpr unsigned int ToGL(TextureFilterMode mode)
	{
		switch (mode)
		{
			case TextureFilterMode::LINEAR:						return GL_LINEAR;
			case TextureFilterMode::NEAREST:					return GL_NEAREST;
			case TextureFilterMode::MIPMAP_LINEAR_LINEAR:		return GL_LINEAR_MIPMAP_LINEAR;
			case TextureFilterMode::MIPMAP_LINEAR_NEAREST:		return GL_LINEAR_MIPMAP_NEAREST;
			case TextureFilterMode::MIPMAP_NEAREST_NEAREST:		return GL_NEAREST_MIPMAP_NEAREST;
			case TextureFilterMode::MIPMAP_NEAREST_LINEAR:		return GL_NEAREST_MIPMAP_LINEAR;
		}
		return GL_LINEAR;
	}

}

#endif // !SRC_GRAPHICS_COMMON_GL_TYPES_INL_