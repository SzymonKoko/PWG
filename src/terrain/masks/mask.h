#ifndef SRC_TERRAIN_MASKS_MASK_H_
#define SRC_TERRAIN_MASKS_MASK_H_

#include <string>
#include "graphics/texture/texture.h"

namespace pwg
{
	enum class MaskUsage
	{
		HEIGHT,
		WEIGHTS,
		DEBUG
	};

	struct TerrainMask
	{
		std::string name;
		std::shared_ptr<Texture> texture;
		MaskUsage usage;
		float weight{ 1.0 };

		TerrainMask(std::string name, std::shared_ptr<Texture> texture, MaskUsage usage, float weight)
			: name(name),
			  texture(texture),
			  usage(usage),
			  weight(weight)
		{}
	};
}

#endif // !SRC_TERRAIN_MASKS_MASK_H_
