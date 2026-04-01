#ifndef SRC_GRAPHICS_RENDERER_IRENDERABLE_H_
#define SRC_GRAPHICS_RENDERER_IRENDERABLE_H_

#include "graphics/renderer/renderer.h"

namespace pwg
{
	/**
	* @brief Interface for objects that can be rendered.
	* Defines the contract for updating and drawing renderable entities.
	*/
	class IRenderable
	{
	public:
		/**
		* @brief Updates the renderable object state.
		* @param dt Delta time since last update (in seconds).
		*/
		virtual void Update(float dt, std::shared_ptr<ICamera> camera) = 0;

		/**
		* @brief Draws the renderable object using the provided renderer.
		* @param renderer Reference to the renderer responsible for drawing.
		*/
		virtual void Draw(Renderer& renderer) = 0;

		/**
		* @brief Retrieves the mesh associated with this renderable.
		* @return Shared pointer to the mesh.
		*/
		virtual std::shared_ptr<Mesh> GetMesh() = 0;

		/**
		* @brief Retrieves the material associated with this renderable.
		* @return Shared pointer to the material.
		*/
		virtual std::shared_ptr<Material> GetMaterial() = 0;

	};
}

#endif // !SRC_GRAPHICS_RENDERER_IRENDERABLE_H_
