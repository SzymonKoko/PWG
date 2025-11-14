#ifndef SRC_GRAPHICS_RENDERER_RENDERER_H_
#define SRC_GRAPHICS_RENDERER_RENDERER_H_

#include "graphics/shader/shader.h"
#include "graphics/texture/texture.h"
#include "scene/components/cameraComponent.h"
#include "graphics/mesh/mesh.h"
#include "resources/resourceManager.h"
#include "terrain/terrain.h"

namespace pwg
{
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
		Renderer(std::shared_ptr<ResourceManager> resourceManager);

		/**
		* @brief Destructor. Cleans up renderer resources.
		*/
		~Renderer();

		/**
		* @brief Clears the screen before rendering a new frame.
		* Typically clears color and depth buffers.
		*/
		void Clear();

		/**
		* @brief Updates renderer state for the current frame.
		* Can update camera uniforms and mesh data before drawing.
		* @param camera Pointer to a CameraComponent providing view/projection matrices.
		* @param mesh Reference to the mesh being rendered.
		*/
		void Update(pwg::components::CameraComponent* camera, Mesh& mesh);

		/**
		* @brief Draws a given mesh using the currently bound shader and resources.
		* @param mesh Reference to the mesh to draw.
		*/
		void Draw(Terrain& terrain);

	private:
		std::shared_ptr<ResourceManager> m_resourceManager; /**< Access to resources like shaders, textures, and meshes. */
		std::shared_ptr<Shader> m_currentShader{ nullptr }; /**< Currently bound shader for rendering. */
	};
} // namespace pwg

#endif // !SRC_GRAPHICS_RENDERER_RENDERER_H_
