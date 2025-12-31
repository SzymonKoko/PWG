#ifndef SRC_SCENE_OBJECTS_SUN_OBJECT_H_
#define SRC_SCENE_OBJECTS_SUN_OBJECT_H_

#include "graphics/renderer/irenderable.h"
#include "resources/resourceManager.h"

namespace pwg
{
	class SunObject : public IRenderable
	{
	public:
		SunObject(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);
		~SunObject();

		void Update(float dt) override;

		void Draw(Renderer& renderer) override;

		std::shared_ptr<Mesh> GetMesh() override;

		std::shared_ptr<Material> GetMaterial() override;

		glm::vec3 GetPosition();

		glm::vec3 GetColor();

	private:
		std::shared_ptr<Mesh> m_mesh;
		std::shared_ptr<Material> m_material;

		float m_orbitRadius{ 2000.f };
		float m_orbitSpeed{ 0.1f };
		float m_time{ 0.0f };

		glm::vec3 m_color{ 1.0, 1.0, 0.3 };
		glm::vec3 m_position{ 0.0f, 0.0f, 0.0f };

	};
}

#endif // !SRC_SCENE_OBJECTS_SUN_OBJECT_H_
