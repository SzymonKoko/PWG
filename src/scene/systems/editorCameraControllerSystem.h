#ifndef SRC_SCENE_SYSTEMS_EDITOR_CAMERA_CONTROLLER_SYSTEM
#define SRC_SCENE_SYSTEMS_EDITOR_CAMERA_CONTROLLER_SYSTEM

#include <entt/entt.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/input/mouseInput.h"
#include "core/input/keyboardInput.h"
#include "scene/components/cameraComponent.h"
#include "scene/components/transformComponent.h"
#include "scene/components/editorCameraComponent.h"

namespace pwg::systems
{
	/**
	 * @brief System that controls an editor-style camera.
	 * Handles rotation, zoom, and positioning based on mouse input.
	 */
	class EditorCameraControllerSystem
	{
	public:

		/**
		 * @brief Constructor for the system.
		 */
		EditorCameraControllerSystem();

		/**
		 * @brief Destructor for the system.
		 */
		~EditorCameraControllerSystem() = default;

		/**
		 * @brief Updates all editor cameras in the registry based on mouse input.
		 * @param registry Reference to the ECS registry.
		 * @param mouseInput Reference to the MouseInput instance.
		 * @param aspectRatio Aspect ratio of the viewport (width / height).
		 */
		static void Update(entt::registry& registry, pwg::MouseInput& mouseInput, pwg::KeyboardInput& keyboardInput, float aspectRatio, float dt);

		/**
		 * @brief Sets the default position of the editor camera based on the scene size.
		 * @param registry Reference to the ECS registry.
		 * @param size Size of the scene or plane mesh.
		 */
		static void SetCameraDefaultPosition(entt::registry& registry, int size);

	private:

		/**
		 * @brief Handles mouse input to rotate and zoom the camera.
		 * @param mouseInput Reference to the MouseInput instance.
		 * @param camera Reference to the CameraComponent of the entity.
		 * @param transform Reference to the TransformComponent of the entity.
		 * @param editorData Reference to the EditorCameraComponent of the entity.
		 */
		static void HandleMouseInput(pwg::MouseInput& mouseInput, pwg::components::CameraComponent& camera, pwg::components::TransformComponent& transform, pwg::components::EditorCameraComponent& editorData);

		static void HandleKeyboardInput(pwg::KeyboardInput& keyboardInput, pwg::components::TransformComponent& transform, float dt, pwg::components::EditorCameraComponent& editorData, pwg::components::CameraComponent& camera);

		static void CalculateCameraForward(pwg::components::EditorCameraComponent& editorData, pwg::components::CameraComponent camera);

		/**
		 * @brief Updates the projection matrix of the camera based on the aspect ratio.
		 * @param camera Reference to the CameraComponent of the entity.
		 * @param aspectRatio Aspect ratio of the viewport (width / height).
		 */
		static void HandleProjection(pwg::components::CameraComponent& camera, float aspectRatio);
	};
} // namespace pwg::systems

#endif // !SRC_SCENE_SYSTEMS_EDITOR_CAMERA_CONTROLLER_SYSTEM
