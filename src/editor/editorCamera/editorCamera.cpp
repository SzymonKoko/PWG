#include "editorCamera.h"
#include "imgui.h"

pwg::EditorCamera::EditorCamera(GLFWwindow* window, MouseInput& minput, KeyboardInput& kinput)
    : m_window(window),
      m_mouseInput(minput),
      m_keyboardInput(kinput),
      m_yaw(-45.0f),
      m_pitch(45.0f),
      m_target(-16.0f, 0.0f, 16.0f),
      m_distanceToTarget(200.0f)
{
    float yawRad = glm::radians(m_yaw);
    float pitchRad = glm::radians(m_pitch);

    m_position.x = m_target.x + m_distanceToTarget * cos(pitchRad) * cos(yawRad);
    m_position.y = m_target.y + m_distanceToTarget * sin(pitchRad);
    m_position.z = m_target.z + m_distanceToTarget * cos(pitchRad) * sin(yawRad);

    UpdateViewMatrix();
    UpdateProjectionMatrix(m_aspectRatio);
}

void pwg::EditorCamera::Update()
{
    HandleMouseInput();
    HandleKeyboardInput();
    UpdateViewMatrix();
}

void pwg::EditorCamera::UpdateViewMatrix()
{
    m_viewMatrix = glm::lookAt(m_position, m_target, m_up);
}

void pwg::EditorCamera::UpdateProjectionMatrix(float aspectRatio)
{

    if (aspectRatio < 0.001f)
    {
        aspectRatio = ICamera::DEFAULT_ASPECT;
    }

    m_aspectRatio = aspectRatio;

    m_projectionMatrix = glm::perspective(
        glm::radians(m_fov),
        m_aspectRatio,
        0.1f,       // near plane
        1000.0f     // far plane
    );
}

void pwg::EditorCamera::SetPosition(const glm::vec3& position)
{
    m_position = position;
    UpdateViewMatrix();
}

glm::vec3& pwg::EditorCamera::GetPosition()
{
    return m_position;
}

void pwg::EditorCamera::HandleMouseInput()
{

   

    if (m_mouseInput.IsButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
    {
        HandleOrbit();
    }
    else if (m_mouseInput.IsButtonDown(GLFW_MOUSE_BUTTON_MIDDLE))
    {
        HandlePan();
    }

    double scroll = m_mouseInput.GetScrollOffsetY();
    
    if (scroll != 0.0f)
    {
       HandleZoom(scroll);
    }
}

void pwg::EditorCamera::HandleKeyboardInput()
{
}

void pwg::EditorCamera::HandleOrbit()
{
    const float sensitivity = 0.1f;

    //Moving camera with mouse
    float xoffset = m_mouseInput.GetDeltaX() * sensitivity;
    float yoffset = m_mouseInput.GetDeltaY() * sensitivity;

    //Updating yaw and pitch angles
    m_yaw += xoffset;
    m_pitch += yoffset;

    //Limit pitch to avoid upside down camera moving
    if (m_pitch > 89.0f)
        m_pitch = 89.0f;
    if (m_pitch < -89.0f)
        m_pitch = -89.0f;

    //Converting camera front angles to vectors
    float yawRad = glm::radians(m_yaw);
    float pitchRad = glm::radians(m_pitch);

    //Setting new position
    m_position.x = m_target.x + m_distanceToTarget * cos(pitchRad) * cos(yawRad);
    m_position.y = m_target.y + m_distanceToTarget * sin(pitchRad);
    m_position.z = m_target.z + m_distanceToTarget * cos(pitchRad) * sin(yawRad);

    //Updating view matrix
    UpdateViewMatrix();
}

void pwg::EditorCamera::HandlePan()
{
    float panSpeed = 0.0005f * m_distanceToTarget;

    float dx = m_mouseInput.GetDeltaX();
    float dy = m_mouseInput.GetDeltaY();

    glm::vec3 right = glm::normalize(glm::cross(m_position - m_target, m_up));
    glm::vec3 upMove = glm::normalize(m_up);

    glm::vec3 pan = (-right * dx + upMove * dy) * panSpeed;

    m_target += pan;
    m_position += pan;

    UpdateViewMatrix();
}

void pwg::EditorCamera::HandleZoom(double scroll)
{
    m_distanceToTarget -= scroll * m_zoomSpeed;
    m_distanceToTarget = glm::clamp(m_distanceToTarget, m_minZoom, m_maxZoom);

    UpdateViewMatrix();
    m_mouseInput.ResetScrollOffset();
}
