#ifndef SRC_GRAPHICS_RENDERER_LIGHTING_UPLOADER_H_
#define SRC_GRAPHICS_RENDERER_LIGHTING_UPLOADER_H_

#include "graphics/material/material.h"
#include "scene/lights/light.h"

namespace pwg
{
	class LightingUploader
	{
	public:
		LightingUploader() = default;
		~LightingUploader() = default;

		void Upload(std::shared_ptr<Material> material, Light& light);
	private:

	};
}
#endif // !SRC_GRAPHICS_RENDERER_LIGHTING_UPLOADER_H_
