#include "editorCamera.h"

pwg::EditorCamera::EditorCamera()
{
	m_transform.position = { 0.0f, 0.0f, 0.0f };
}

void pwg::EditorCamera::Update(float dt, pwg::MouseInput& mouseInput, pwg::KeyboardInput& keyboardInput)
{
    CalculateCameraForward();
    HandleMouseInput(mouseInput, dt);
    m_cameraProperties.projectionMatrix = glm::perspective(glm::radians(m_cameraProperties.fov), m_cameraProperties.aspectRatio, m_cameraProperties.nearPlane, m_cameraProperties.farPlane);
}

void pwg::EditorCamera::SetDefaultCameraPosition(int size)
{
    m_transform.position.x = (float)size + 50.0f;
    m_transform.position.y = (m_transform.position.x / 50.0f) * 70;
    m_transform.position.z = -((float)size + 50.0f);

    m_editorCameraProperties.distanceToTarget = m_transform.position.x * 2;

    m_cameraProperties.viewMatrix = glm::lookAt(m_transform.position, m_editorCameraProperties.target, m_cameraProperties.up);

    PWG_INFO("Camera set to default (Position: {0} {1} {2}, distance to target: {3}, target: {4} {5} {6})",
        m_transform.position.x,
        m_transform.position.y,
        m_transform.position.z,
        m_editorCameraProperties.distanceToTarget,
        m_editorCameraProperties.target.x,
        m_editorCameraProperties.target.y,
        m_editorCameraProperties.target.z
    );
}

glm::mat4& pwg::EditorCamera::GetViewMatrix()
{
    return m_cameraProperties.viewMatrix;
}

glm::mat4& pwg::EditorCamera::GetProjectionMatrix()
{
    return m_cameraProperties.projectionMatrix;
}

void pwg::EditorCamera::HandleKeyboardInput(pwg::KeyboardInput& keyboardInput, float dt)
{
}

void pwg::EditorCamera::HandleMouseInput(pwg::MouseInput& mouseInput, float dt)
{
    float deltaX = mouseInput.GetDeltaX();
    float deltaY = mouseInput.GetDeltaY();

    if (mouseInput.IsButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
    {
        const float sensitivity = 0.1f;

        //Moving m_cameraProperties with mouse
        float xoffset = deltaX * sensitivity;
        float yoffset = deltaY * sensitivity;

        //Updating yaw and pitch angles
        m_editorCameraProperties.yaw += xoffset;
        m_editorCameraProperties.pitch += yoffset;
        m_editorCameraProperties.pitch = glm::clamp(m_editorCameraProperties.pitch, -89.0f, 89.0f);
    }
    else if (mouseInput.IsButtonDown(GLFW_MOUSE_BUTTON_MIDDLE))
    {
        float panSpeed = 0.0005f * m_editorCameraProperties.distanceToTarget;

        glm::vec3 right = glm::normalize(glm::cross(m_cameraProperties.forward, m_cameraProperties.up));

        glm::vec3 pan = (-right * deltaX + m_cameraProperties.up * deltaY) * panSpeed;

        m_editorCameraProperties.target += pan;
    }

    double scroll = mouseInput.GetScrollOffsetY();

    if (scroll != 0.0f)
    {
        m_editorCameraProperties.distanceToTarget -= scroll * m_editorCameraProperties.zoomSpeed;
        //m_editorCameraProperties.distanceToTarget = glm::clamp(m_editorCameraProperties.distanceToTarget, m_editorCameraProperties.minZoom, m_editorCameraProperties.maxZoom);

        mouseInput.ResetScrollOffset();
    }

    //Converting m_cameraProperties front angles to vectors
    float yawRad = glm::radians(m_editorCameraProperties.yaw);
    float pitchRad = glm::radians(m_editorCameraProperties.pitch);


    //Setting new position
    m_transform.position.x = m_editorCameraProperties.target.x + m_editorCameraProperties.distanceToTarget * cos(pitchRad) * cos(yawRad);
    m_transform.position.y = m_editorCameraProperties.target.y + m_editorCameraProperties.distanceToTarget * sin(pitchRad);
    m_transform.position.z = m_editorCameraProperties.target.z + m_editorCameraProperties.distanceToTarget * cos(pitchRad) * sin(yawRad);

    m_cameraProperties.viewMatrix = glm::lookAt(m_transform.position, m_editorCameraProperties.target, m_cameraProperties.up);
}

void pwg::EditorCamera::CalculateCameraForward()
{
    float yawRad = glm::radians(m_editorCameraProperties.yaw);
    float pitchRad = glm::radians(m_editorCameraProperties.pitch);

    m_cameraProperties.forward = glm::normalize(glm::vec3(cos(pitchRad) * cos(yawRad), sin(pitchRad), cos(pitchRad) * sin(yawRad)));
}
