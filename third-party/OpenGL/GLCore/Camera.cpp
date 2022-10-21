#include "Camera.h"
#include <GLFW/glfw3.h>

Camera::Camera(GLFWwindow *window) :
    window(window),
    cameraDirection(glm::normalize(cameraPos - cameraTarget)),
    cameraRight(glm::normalize(glm::cross(UP, cameraDirection))),
    cameraUp(glm::cross(cameraDirection, cameraRight))
{ 
    updateView();
    updateProjection();
}

void Camera::processKeyboardInput(float deltaTime)
{
    cameraSpeed = 2.5f * deltaTime;
    bool update = false;
    update |= glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    update |= glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    update |= glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    update |= glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraFront * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraFront * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    if (update)
        updateView();
}

void Camera::processMousePosUpdate(double xpos, double ypos)
{
    bool mouseControl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)  == GLFW_PRESS ||
                        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
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
    proj = glm::perspective(glm::radians(fov), (float)width / (float)height, -1.0f, 100.0f);
}

void Camera::updateView()
{
    updateCameraVectors();
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

void Camera::updateCameraVectors()
{
    // calculate the new Front vector
    glm::vec3 front{};
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
    // also re-calculate the Right and Up vector
    cameraRight = glm::normalize(glm::cross(cameraFront, UP));
    cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));
}