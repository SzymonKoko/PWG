#ifndef SRC_GRAPHICS_RENDERER_RENDERER_H_
#define SRC_GRAPHICS_RENDERER_RENDERER_H_

#include "graphics/shader/shader.h"
#include "graphics/texture/texture.h"
#include "scene/components/cameraComponent.h"
#include "graphics/mesh/mesh.h"
#include "resources/resourceManager.h"
#include "lightingUploader.h"
#include "scene/cameras/icamera.h"

namespace pwg
{
	class IRenderable;
	/**
	* @brief Class responsible for rendering meshes using shaders and textures.
	* Handles basic draw calls, clearing the screen, and shader management.
	*/
	class Renderer
	{
	public:

		/**
		* @brief Constructs a renderer with access to the resource manager.
		* @param resourceManager Shared pointer to the ResourceManager for shaders, textures, and meshes.
		*/
		Renderer();

		/**
		* @brief Destructor. Cleans up renderer resources.
		*/
		~Renderer();

		/**
		* @brief Clears the screen before rendering a new frame.
		* Typically clears color and depth buffers.
		*/
		void BeginFrame();

		void EndFrame();

		/**
		* @brief Draws a given renderable object.
		* @param renderable Pointer to the renderable objcet to draw.
		*/
		void Draw(IRenderable* renderable);

		/**
		* @brief Draws a given renderable objects vector.
		* @param renderQueue Vector of pointers to the renderable objects to draw.
		*/
		void DrawAll();

		void AddToQueue(IRenderable* renderable);
		void AddLight(Light& light);

		void ClearQueue();

		void SetCamera(std::shared_ptr<ICamera> camera);
	private:
		std::vector<IRenderable*> m_renderQueue;
		std::unique_ptr<LightingUploader> m_lightingUploader;

		glm::mat4 m_viewMatrix{ 1.0f };
		glm::mat4 m_projectionMatrix{ 1.0f };

		Light m_light;
	};
} // namespace pwg

#endif // !SRC_GRAPHICS_RENDERER_RENDERER_H_

