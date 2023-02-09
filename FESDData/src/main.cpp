/// Main.cpp
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <functional>
#include <fstream>
#include <string>
#include <sstream>

#include "GLCore/Renderer.h"
#include "GLCore/GLObject.h"
#include "GLCore/GLErrorManager.h"
#include "GLCore/Camera.h"

#include "cameras/CameraHandler.h"

#include "utilities/Status.h"
#include "obj/Logger.h"

#include "utilities/helper/GLFWHelper.h"
#include "utilities/helper/ImGuiHelper.h"
#include "utilities/WindowInfo.h"
#include "samples/nuitrack_sample.h"
Camera *cam = nullptr;

void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xpos, double ypos);

int main(void)
{
    STATUS status;
    GLFWwindow *window = InitialiseGLFWWindow(status);

    if (status == ERR)
    {
        glfwTerminate();
        return -1;
    }

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    {
        ImGuiHelper::initImGui(window);

        Logger::Logger logger;
        logger.log("Initialised Log");

        cam = new Camera{window};
        
        Renderer r;
        WindowInformation wi{};
        CameraHandler cameraHandler{cam, &r, &logger};

        float deltaTime = 0.0f;	// Time between current frame and last frame
        float lastFrame = 0.0f; // Time of last frame
        
        while (!glfwWindowShouldClose(window))
        {
            float currentFrame = (float)glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            wi.UpdateFps(deltaTime);
            r.Clear();
            
            ImGuiHelper::beginFrame();
            ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

            cam->processKeyboardInput(deltaTime);
            cam->updateImGui();

            wi.ShowInformation();

            cameraHandler.OnUpdate();
            cameraHandler.OnImGuiRender();

            logger.showLog();

            ImGuiHelper::endFrame();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    ImGuiHelper::terminateImGui();

    glfwTerminate();
    return 0;
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    cam->processMousePosUpdate(xpos, ypos);
}

void scroll_callback(GLFWwindow *window, double xpos, double ypos)
{
    cam->processScroll(xpos, ypos);
}
