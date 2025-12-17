#ifndef SRC_SCENE_COMPONENTS_EDITOR_CAMERA_COMPONENT
#define SRC_SCENE_COMPONENTS_EDITOR_CAMERA_COMPONENT

#include <glm/glm.hpp>

namespace pwg::components
{
	/**
	 * @brief Component representing an editor-style camera.
	 * Stores rotation, zoom, and target information for editor camera controls.
	 */
	struct EditorCameraComponent
	{
		static constexpr const char* Name = "Editor Camera Component"; /**< Component name identifier. */

		float yaw{ -45.0f };					/**< Yaw rotation angle in degrees. */
		float pitch{ 45.0f };					/**< Pitch rotation angle in degrees. */
		float distanceToTarget{ 200.0f };		/**< Distance from the camera to the target point. */
		glm::vec3 target{ 0.0f, 0.0f, 0.0f };	/**< Target point the camera is looking at. */

		float minZoom{ 5.0f };					/**< Minimum zoom distance. */
		float maxZoom{ 700.0f };				/**< Maximum zoom distance. */
		float zoomSpeed{ 50.0f };				/**< Speed factor for zooming in/out. */
	};
} // namespace pwg::components

#endif // !SRC_SCENE_COMPONENTS_EDITOR_CAMERA_COMPONENT
