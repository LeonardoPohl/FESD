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

#include "utilities/Consts.h"

#include "utilities/GLFWHelper.h"
#include "utilities/ImGuiHelper.h"
#include "utilities/TestMenuHelper.h"

Camera *cam = nullptr;

void window_size_callback(GLFWwindow *window, int width, int height);
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

    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    {
    
        ImGuiHelper::initImGui(window);
        
        // TODO
        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        GLCall(glClearColor(0.15f, 0.15f, 0.15f, 1.0f));

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui::GetIO().Fonts->AddFontDefault();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init((char *)glGetString(GL_NUM_SHADING_LANGUAGE_VERSIONS));
        

        cam = new Camera{window};
        
        Renderer r;

        TestMenuHelper tmh{ cam };
        
        //# Camera Initialisation
        //#######################
        CameraHandler cameraHandler{cam};

        float deltaTime = 0.0f;	// Time between current frame and last frame
        float lastFrame = 0.0f; // Time of last frame

        while (!glfwWindowShouldClose(window))
        {
            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            r.Clear();
            
            ImGuiHelper::beginFrame();

            //# Test window
            //#############
            cam->processKeyboardInput(deltaTime);
            cam->updateImGui();
            
            {
                if (currentTest)
                {
                    currentTest->OnUpdate();
                    currentTest->OnRender();

                    ImGui::Begin("Test");
                    if (currentTest != testMenu && ImGui::Button("<-"))
                    {
                        delete currentTest;
                        currentTest = testMenu;
                    }
                    currentTest->OnImGuiRender();
                    ImGui::End();
                }
            }

            //# General Camera Window
            //#######################

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            {
                ImGui::Begin("Camera Handler");

                if (ImGui::Button("Init Cameras"))
                {
                    // TODO: Make Async
                    cameraHandler.initAllCameras();
                }
                cameraHandler.OnImGuiRender();
                //cameraHandler.showCameras();

                ImGui::End();
            }

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            
            glfwSwapBuffers(window);

            tmh.update();

            cameraHandler.OnImGuiRender();

            ImGuiHelper::endFrame();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    ImGuiHelper::terminateImGui();

    glfwTerminate();
    return 0;
}

void window_size_callback(GLFWwindow *window, int width, int height)
{
    WINDOW_WIDTH = width;
    WINDOW_HEIGHT = height;
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    cam->processMousePosUpdate(xpos, ypos);
}

void scroll_callback(GLFWwindow *window, double xpos, double ypos)
{
    cam->processScroll(xpos, ypos);
}
