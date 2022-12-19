/// Main.cpp
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <functional>
#include <fstream>
#include <string>
#include <sstream>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "GLCore/Renderer.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "GLCore/GLObject.h"
#include "GLCore/GLErrorManager.h"
#include "GLCore/Camera.h"

#include <OpenNI.h>
#include "cameras/CameraHandler.h"

#include "utilities/Status.h"
#include "obj/Logger.h"

#include "utilities/helper/GLFWHelper.h"
#include "utilities/helper/ImGuiHelper.h"
#include "utilities/helper/TestMenuHelper.h"

Camera *cam = nullptr;

// void window_size_callback(GLFWwindow *window, int width, int height);
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

    //glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    {
        ImGuiHelper::initImGui(window);

        Logger::Logger logger;
        logger.log(Logger::LogLevel::INFO, "Initialised Log");

        cam = new Camera{window};
        
        Renderer r;

        TestMenuHelper tmh{ cam };
        bool showTestMenu = false;
        //# Camera Initialisation
        //#######################
        CameraHandler cameraHandler{cam, &r, &logger};

        float deltaTime = 0.0f;	// Time between current frame and last frame
        float lastFrame = 0.0f; // Time of last frame
        float fps = 0.0f;
        float fpsSmoothing = 0.9f;

        while (!glfwWindowShouldClose(window))
        {
            float currentFrame = (float)glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            fps = (float)((fps * fpsSmoothing) + (deltaTime * (1.0 - fpsSmoothing)));

            r.Clear();
            
            ImGuiHelper::beginFrame();

            //# Test window
            //#############
            cam->processKeyboardInput(deltaTime);
            cam->updateImGui();

            ImGui::Begin("Information");

            ImGui::Text("FPS: %.2f", 1.0f/fps);

            ImGui::End();

            if (showTestMenu)
                tmh.update();

            cameraHandler.OnRender();
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
