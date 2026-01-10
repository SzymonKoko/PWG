#ifndef SRC_SCENE_CAMERAS_CAMERA_MANAGER_H_
#define SRC_SCENE_CAMERAS_CAMERA_MANAGER_H_

#include "editorCamera.h"
#include "playerCamera.h"
#include <memory>
namespace pwg
{
	enum class CameraType
	{
		EDITOR = 0,
		PLAYER
	};

	class CameraManager
	{
	public:
		CameraManager(pwg::MouseInput& mouseInput, pwg::KeyboardInput& keyboardInput);
		~CameraManager() = default;

		void Update(float dt);

		void SetCamera(CameraType type);

		void SetDefaultCameraPosition(int size);
		
		std::shared_ptr<ICamera> GetActiveCamera();

	private:
		MouseInput& m_mouseInput;
		KeyboardInput& m_keyboardInput;

		std::shared_ptr<EditorCamera> m_editorCamera;
		//shared ptr player cam
		std::shared_ptr<ICamera> m_activeCamera;
	};
}
#endif // !SRC_SCENE_CAMERAS_CAMERA_MANAGER_H_
