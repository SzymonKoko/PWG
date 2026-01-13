#ifndef SRC_SCENE_OBJECTS_SUN_OBJECT_H_
#define SRC_SCENE_OBJECTS_SUN_OBJECT_H_

#include "graphics/renderer/irenderable.h"
#include "resources/resourceManager.h"
#include "scene/lights/light.h"

namespace pwg
{
	/**
	 * @brief Properties controlling the behavior and appearance of the Sun object.
	 */
	struct SunObjectProperties
	{
		float orbitRadius{ 2000.f };				/**< Radius of the sun's orbital path. */
		float orbitSpeed{ 0.1f };					/**< Speed at which the sun orbits. */
		float time{ 0.0f };							/**< Current time value used for orbit calculation. */

		glm::vec3 color{ 1.0f, 1.0f, 0.3f };		/**< Color of the sun. */
		glm::vec3 position{ 0.0f, 0.0f, 0.0f };		/**< Current position of the sun in world space. */
	};

	/**
	 * @brief Represents a Sun object in the scene.
	 * Handles orbiting, rendering, and providing light data.
	 */
	class SunObject : public IRenderable
	{
	public:
		/**
		 * @brief Constructs a SunObject with a mesh and material.
		 * @param mesh Shared pointer to the mesh representing the sun.
		 * @param material Shared pointer to the material used to render the sun.
		 */
		SunObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);

		/**
		 * @brief Destructor.
		 */
		~SunObject();

		/**
		 * @brief Updates the sun's position based on orbit speed and elapsed time.
		 * @param dt Delta time since last frame.
		 */
		void Update(float dt) override;

		/**
		 * @brief Draws the sun object using the provided renderer.
		 * @param renderer Reference to the renderer.
		 */
		void Draw(Renderer& renderer) override;

		/**
		 * @brief Returns the mesh of the sun.
		 * @return Shared pointer to the sun's mesh.
		 */
		std::shared_ptr<Mesh> GetMesh() override;

		/**
		 * @brief Returns the material of the sun.
		 * @return Shared pointer to the sun's material.
		 */
		std::shared_ptr<Material> GetMaterial() override;

		/**
		 * @brief Returns the current position of the sun in world space.
		 * @return Sun position vector.
		 */
		glm::vec3 GetPosition();

		/**
		 * @brief Returns the color of the sun.
		 * @return Sun color vector.
		 */
		glm::vec3 GetColor();

		/**
		 * @brief Returns the light data associated with the sun.
		 * @return Reference to the Light struct.
		 */
		Light& GetLight();

		/**
		 * @brief Returns the properties of the sun object.
		 * @return Reference to the SunObjectProperties struct.
		 */
		SunObjectProperties& GetSunObjectProperties();

	private:
		std::shared_ptr<Mesh> m_mesh;              /**< Mesh representing the sun. */
		std::shared_ptr<Material> m_material;      /**< Material for rendering the sun. */
		Light m_light;                             /**< Light emitted by the sun. */
		SunObjectProperties m_sunObjectProperties; /**< Properties controlling orbit, color, and position. */
	};
} // namespace pwg

#endif // !SRC_SCENE_OBJECTS_SUN_OBJECT_H_
