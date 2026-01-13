#ifndef SRC_GRAPHICS_RENDERER_LIGHTING_UPLOADER_H_
#define SRC_GRAPHICS_RENDERER_LIGHTING_UPLOADER_H_

#include "graphics/material/material.h"
#include "scene/lights/light.h"

namespace pwg
{
	/**
	* @brief Utility class responsible for uploading lighting data to materials.
	* Handles transferring light properties and camera position to shaders for rendering.
	*/
	class LightingUploader
	{
	public:
		/**
		* @brief Default constructor.
		*/
		LightingUploader() = default;

		/**
		* @brief Default destructor.
		*/
		~LightingUploader() = default;

		/**
		* @brief Uploads lighting information to a material.
		* @param material Shared pointer to the material to update.
		* @param light Reference to the light whose properties will be uploaded.
		* @param cameraPosition World-space position of the camera for specular calculations.
		*/
		void Upload(std::shared_ptr<Material> material, Light& light, glm::vec3 cameraPosition);

	private:
		// No private members currently
	};
}

#endif // !SRC_GRAPHICS_RENDERER_LIGHTING_UPLOADER_H_
