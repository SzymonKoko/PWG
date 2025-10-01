#ifndef SRC_SCENE_SYSTEMS_EDITOR_CAMERA_CONTROLLER_SYSTEM
#define SRC_SCENE_SYSTEMS_EDITOR_CAMERA_CONTROLLER_SYSTEM

#include <entt/entt.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/input/mouseInput.h"
#include "scene/components/cameraComponent.h"
#include "scene/components/transformComponent.h"
#include "scene/components/editorCameraComponent.h"

namespace pwg::systems
{
	class EditorCameraControllerSystem
	{
	public:
		EditorCameraControllerSystem();
		~EditorCameraControllerSystem() = default;

		static void Update(entt::registry& registry, pwg::MouseInput& mouseInput, float aspectRatio);
	private:

		static void HandleMouseInput(pwg::MouseInput& mouseInput, pwg::components::CameraComponent& camera, pwg::components::TransformComponent& transform, pwg::components::EditorCameraComponent& editorData);
		static void HandleProjection(pwg::components::CameraComponent& camera, float aspectRatio);
	};
}
#endif // !SRC_SCENE_SYSTEMS_EDITOR_CAMERA_CONTROLLER_SYSTEM
