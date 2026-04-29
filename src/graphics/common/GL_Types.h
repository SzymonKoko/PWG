#ifndef SRC_GRAPHICS_COMMON_GL_TYPES_H_
#define SRC_GRAPHICS_COMMON_GL_TYPES_H_

#include <cstdint>

namespace pwg
{
	/**
	* @brief Enumeration defining available texture wrapping modes.
	* Determines how texture coordinates outside the [0, 1] range are handled.
	*/
	enum class TextureWrapMode : uint8_t
	{
		REPEAT,				/**< Repeats the texture when coordinates exceed range */
		CLAMP_TO_EDGE,		/**< Clamps texture coordinates to the edge texels */
		MIRRORED_REPEAT		/**< Repeats the texture with mirrored tiling */
	};

	/**
	* @brief Enumeration defining available texture filtering modes.
	* Controls how texels are sampled when textures are magnified or minified.
	*/
	enum class TextureFilterMode : uint8_t
	{
		LINEAR,					/**< Linear filtering without mipmaps */
		NEAREST,				/**< Nearest-neighbor filtering without mipmaps */
		MIPMAP_LINEAR_NEAREST,	/**< Linear minification, nearest mipmap selection */
		MIPMAP_LINEAR_LINEAR,	/**< Linear filtering with linear mipmap interpolation (trilinear) */
		MIPMAP_NEAREST_NEAREST, /**< Nearest filtering with nearest mipmap selection */
		MIPMAP_NEAREST_LINEAR	/**< Nearest filtering with linear mipmap interpolation */
	};

	/**
	* @brief Enumeration defining supported texture formats.
	* Used as an engine-level abstraction over OpenGL texture formats.
	*/
	enum class TextureFormats : uint8_t
	{
		R8,			/**< Single-channel 8-bit normalized texture */
		R32F,		/**< Single-channel 32-bit floating-point texture */
		RGBA8,		/**< Four-channel 8-bit normalized texture */
		SRGBA8,
		RGBA16F,	/**< Four-channel 16-bit floating-point texture */
		RGBA32F		/**< Four-channel 32-bit floating-point texture */
	};

	/**
	* @brief Enum representing different types of textures.
	*/
	enum class TextureType : uint8_t
	{
		ALBEDO,
		NORMAL,
		DATA,
		HEIGHT
	};

	/**
	* @brief Structure describing OpenGL texture format parameters.
	* Stores internal format, base format, and data type used for texture creation.
	*/
	struct GLTextureFormats
	{
		int internalFormat;	/**< OpenGL internal texture format */
		int format;			/**< OpenGL base format */
		int type;			/**< OpenGL data type */
	};

	/**
	* @brief Converts an engine texture wrap mode to the corresponding OpenGL value.
	* @param mode Engine-level texture wrap mode.
	* @return OpenGL enum representing the wrap mode.
	*/
	constexpr unsigned int ToGL(TextureWrapMode mode);

	/**
	* @brief Converts an engine texture filter mode to the corresponding OpenGL value.
	* @param mode Engine-level texture filter mode.
	* @return OpenGL enum representing the filter mode.
	*/
	constexpr unsigned int ToGL(TextureFilterMode mode);

	/**
	* @brief Converts an engine texture format to its OpenGL representation.
	* @param format Engine-level texture format.
	* @return Structure containing OpenGL texture format parameters.
	*/
	constexpr GLTextureFormats ToGL(TextureFormats format);

	constexpr GLTextureFormats GetFormatForType(TextureType type);
}

#include "GL_Types.inl"

#endif // !SRC_GRAPHICS_COMMON_GL_TYPES_H_
