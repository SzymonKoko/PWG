#ifndef SRC_GRAPHICS_CAMERA_ICAMERA
#define SRC_GRAPHICS_CAMERA_ICAMERA

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace pwg
{
	class ICamera
	{
	public:
		static constexpr float DEFAULT_FOV = 45.0f;
		static constexpr float DEFAULT_ASPECT = 16.0f / 9.0f;
		static constexpr float DEFAULT_NEAR = 0.1f;
		static constexpr float DEFAULT_FAR = 2000.0f;
		static constexpr glm::vec3 DEFAULT_UP = { 0.0f, 50.0f, 0.0f };

	public:
		ICamera() {};
		virtual ~ICamera() = default;

		virtual void Update() = 0;

		virtual void UpdateViewMatrix() = 0;
		virtual void UpdateProjectionMatrix(float aspectRatio) = 0;
		virtual void SetPosition(const glm::vec3& position) = 0;
		virtual glm::vec3& GetPosition() = 0;

		const glm::mat4& GetViewMatrix() { return m_viewMatrix; }
		const glm::mat4& GetProjectionMatrix() { return m_projectionMatrix; }
		
	protected:
		glm::vec3 m_position;
		glm::vec3 m_front;
		glm::vec3 m_up{ DEFAULT_UP };

		glm::mat4 m_viewMatrix;
		glm::mat4 m_projectionMatrix;

		float m_fov{ DEFAULT_FOV };
		float m_aspectRatio{ DEFAULT_ASPECT };
		float m_nearPlane{ DEFAULT_NEAR };
		float m_farPlane{ DEFAULT_FAR };
	};
}
#endif // !SRC_GRAPHICS_CAMERA_ICAMERA
