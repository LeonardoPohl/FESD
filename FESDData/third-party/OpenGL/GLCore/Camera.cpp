#include "Camera.h"
#include <GLFW/glfw3.h>
#include "imgui/imgui.h"

Camera::Camera(GLFWwindow *window) : window(window)
{ 
    updateView();
    updateProjection();
}

void Camera::processKeyboardInput(float deltaTime)
{
    cameraSpeed = 2.5f * deltaTime * (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS ? 1.0f : 5.0f);
    bool update = false;
    update |= glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    update |= glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    update |= glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    update |= glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        Position += Front * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        Position -= Front * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        Position -= glm::normalize(glm::cross(Front, Up)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        Position += glm::normalize(glm::cross(Front, Up)) * cameraSpeed;

    if (update)
        updateView();
}

void Camera::processMousePosUpdate(double xpos, double ypos)
{
    bool mouseControl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)  == GLFW_PRESS ||
                        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS ||
                        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS;
    if (firstMouse || !mouseControl)
    {
        if (!firstMouse && glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_NORMAL)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    if (mouseControl)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;

        lastX = xpos;
        lastY = ypos;

        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw = glm::mod(yaw + xoffset, 360.0f);
        pitch += yoffset;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        updateView();
    }
}

void Camera::processScroll(double, double yoffset)
{
    bool mouseControl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS ||
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS;

    if (!mouseControl)
        return;

    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 60.0f)
        fov = 60.0f;
    updateProjection();
}

void Camera::updateProjection()
{
    int width;
    int height;
    glfwGetWindowSize(window, &width, &height);
    proj = glm::perspective(glm::radians(fov), (float)width / (float)height, 0.1f, 10.f);
}

void Camera::updateView()
{
    updateCameraVectors();
    view = glm::lookAt(Position, Position + Front, Up);
}

void Camera::updateImGui()
{
    bool update = false;
    ImGui::Begin("Camera Controller");

    if (ImGui::Button("Reset Camera"))
    {
        update = true;
        pitch = PITCH;
        yaw = YAW;
        Position = POS;
    }

    ImGui::Text("Angles");

    update |= ImGui::SliderFloat("Pitch", &pitch, -90.0f, 90.f);
    update |= ImGui::SliderFloat("Yaw", &yaw, 0.0f, 360.f);

    if (ImGui::SliderFloat("Fov", &fov, 1.0f, 60.0f))
    {
        updateProjection();
    }

    ImGui::Separator();
    ImGui::Text("Position");

    update |= ImGui::SliderFloat3("Position", &Position.x, -20.0f, 20.f);

    if (ImGui::TreeNode("Orientation"))
    {
        ImGui::BeginDisabled();

        ImGui::SliderFloat3("Front", &Front.x, -1.0f, 1.f);
        ImGui::SliderFloat3("Right", &Right.x, -1.0f, 1.f);
        ImGui::SliderFloat3("Up", &Up.x, -1.0f, 1.f);

        ImGui::EndDisabled();
    }

    ImGui::End();

    if (update)
    {
        updateView();
    }
}

void Camera::updateCameraVectors()
{
    // calculate the new Front vector
    glm::vec3 front{};
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    Front = glm::normalize(front);
    // also re-calculate the Right and Up vector
    Right = glm::normalize(glm::cross(Front, UP));
    Up = glm::normalize(glm::cross(Right, Front));
}