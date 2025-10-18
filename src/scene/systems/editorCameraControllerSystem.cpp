#include "editorCameraControllerSystem.h"

pwg::systems::EditorCameraControllerSystem::EditorCameraControllerSystem()
{
}

void pwg::systems::EditorCameraControllerSystem::Update(entt::registry& registry, pwg::MouseInput& mouseInput, float aspectRatio)
{
    auto view = registry.view<pwg::components::TransformComponent, pwg::components::CameraComponent, pwg::components::EditorCameraComponent>();

    for (auto [entity, transform, camera, editorData] : view.each())
    {
        //std::cout << transform.position.x << " " << transform.position.y << " " << transform.position.z << "Distance: " << editorData.distanceToTarget << std::endl;
        HandleMouseInput(mouseInput, camera, transform, editorData);
        HandleProjection(camera, aspectRatio);
    }
}

void pwg::systems::EditorCameraControllerSystem::HandleMouseInput(pwg::MouseInput& mouseInput, pwg::components::CameraComponent& camera, pwg::components::TransformComponent& transform, pwg::components::EditorCameraComponent& editorData)
{
    float deltaX = mouseInput.GetDeltaX();
    float deltaY = mouseInput.GetDeltaY();

    if (mouseInput.IsButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
    {
        const float sensitivity = 0.1f;

        //Moving camera with mouse
        float xoffset = deltaX * sensitivity;
        float yoffset = deltaY * sensitivity;

        //Updating yaw and pitch angles
        editorData.yaw += xoffset;
        editorData.pitch += yoffset;
        editorData.pitch = glm::clamp(editorData.pitch, -89.0f, 89.0f);
    }
    else if (mouseInput.IsButtonDown(GLFW_MOUSE_BUTTON_MIDDLE))
    {
        float panSpeed = 0.0005f * editorData.distanceToTarget;

        glm::vec3 right = glm::normalize(glm::cross(transform.position - editorData.target , camera.up));

        glm::vec3 pan = (-right * deltaX + camera.up * deltaY) * panSpeed;

        editorData.target += pan;
    }

    double scroll = mouseInput.GetScrollOffsetY();

    if (scroll != 0.0f)
    {
        editorData.distanceToTarget -= scroll * editorData.zoomSpeed;
        editorData.distanceToTarget = glm::clamp(editorData.distanceToTarget, editorData.minZoom, editorData.maxZoom);

        mouseInput.ResetScrollOffset();
    }

    //Converting camera front angles to vectors
    float yawRad = glm::radians(editorData.yaw);
    float pitchRad = glm::radians(editorData.pitch);

    //Setting new position
    transform.position.x = editorData.target.x + editorData.distanceToTarget * cos(pitchRad) * cos(yawRad);
    transform.position.y = editorData.target.y + editorData.distanceToTarget * sin(pitchRad);
    transform.position.z = editorData.target.z + editorData.distanceToTarget * cos(pitchRad) * sin(yawRad);

    camera.viewMatrix = glm::lookAt(transform.position, editorData.target, camera.up);
}

void pwg::systems::EditorCameraControllerSystem::HandleProjection(pwg::components::CameraComponent& camera, float aspectRatio)
{
    camera.projectionMatrix = glm::perspective(glm::radians(camera.fov), aspectRatio, camera.nearPlane, camera.farPlane);
}

void pwg::systems::EditorCameraControllerSystem::SetCameraDefaultPosition(entt::registry& registry, int size)
{
    auto view = registry.view<pwg::components::TransformComponent, pwg::components::EditorCameraComponent>();

    for (auto [entity, transform, editorData] : view.each())
    {
        transform.position.x = (float)size + 50.0f;
        transform.position.y = (transform.position.x / 50.0f) * 70;
        transform.position.z = -((float)size + 50.0f);

        editorData.distanceToTarget = transform.position.x * 2;
    }
}
