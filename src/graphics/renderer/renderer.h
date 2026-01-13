#ifndef SRC_GRAPHICS_RENDERER_RENDERER_H_
#define SRC_GRAPHICS_RENDERER_RENDERER_H_

#include "graphics/shader/shader.h"
#include "graphics/texture/texture.h"
#include "graphics/mesh/mesh.h"
#include "resources/resourceManager.h"
#include "lightingUploader.h"
#include "scene/cameras/icamera.h"

namespace pwg
{
	class IRenderable;

	/**
	* @brief Class responsible for rendering 3D objects using shaders, textures, and materials.
	* Manages render queues, camera matrices, lighting, and basic draw calls.
	*/
	class Renderer
	{
	public:

		/**
		* @brief Constructs the renderer.
		* Initializes internal state and sets up access to resource management.
		*/
		Renderer();

		/**
		* @brief Destructor for the renderer.
		* Cleans up allocated resources.
		*/
		~Renderer();

		/**
		* @brief Begins a new frame by clearing the screen.
		* Typically clears the color and depth buffers.
		*/
		void BeginFrame();

		/**
		* @brief Ends the current frame.
		* Can be used for post-processing or buffer swapping.
		*/
		void EndFrame();

		/**
		* @brief Draws a single renderable object.
		* @param renderable Pointer to the renderable object to draw.
		*/
		void Draw(IRenderable* renderable);

		/**
		* @brief Draws all renderable objects in the current render queue.
		*/
		void DrawAll();

		/**
		* @brief Adds a renderable object to the render queue.
		* @param renderable Pointer to the renderable object.
		*/
		void AddToQueue(IRenderable* renderable);

		/**
		* @brief Adds a light to the scene for lighting calculations.
		* @param light Reference to the light to add.
		*/
		void AddLight(Light& light);

		/**
		* @brief Clears the current render queue.
		* Should be called at the end of each frame.
		*/
		void ClearQueue();

		/**
		* @brief Sets the active camera used for rendering.
		* @param camera Shared pointer to a camera implementing ICamera.
		*/
		void SetCamera(std::shared_ptr<ICamera> camera);

	private:
		std::vector<IRenderable*> m_renderQueue;				/**< Container storing pointers to all renderable objects for the current frame */
		std::unique_ptr<LightingUploader> m_lightingUploader;	/**< Utility for uploading lighting information to materials */

		glm::mat4 m_viewMatrix{ 1.0f };							/**< Cached view matrix from the active camera */
		glm::mat4 m_projectionMatrix{ 1.0f };					/**< Cached projection matrix from the active camera */
		glm::vec3 m_cameraPosition{ 0.0f };						/**< World-space position of the camera */

		Light m_light;											/**< Single scene light (extendable for multiple lights) */
	};
} // namespace pwg

#endif // !SRC_GRAPHICS_RENDERER_RENDERER_H_
