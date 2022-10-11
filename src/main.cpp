/// Main.cpp
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <fstream>
#include <string>
#include <sstream>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "GLCore/Renderer.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "obj/tests/TestClearColor.h"
#include "obj/tests/TestTriangle2D.h"
#include "obj/tests/TestTexture2D.h"
#include "obj/tests/TestPyramid3D.h"

#include "GLCore/GLObject.h"
#include "GLCore/GLErrorManager.h"

#include <OpenNI.h>
#include "cameras/CameraHandler.h"

int main(void)
{
    GLFWwindow *window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(960, 540, "Hello World", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK)
    {
        std::cout << "Error!" << std::endl;
    }

    std::cout << glGetString(GL_VERSION) << std::endl;

    {
        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init((char *)glGetString(GL_NUM_SHADING_LANGUAGE_VERSIONS));

        auto io = &ImGui::GetIO();
        io->Fonts->AddFontDefault();
        io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        Renderer r;
        GLObject::GLObject *currentTest;
        GLObject::TestMenu *testMenu = new GLObject::TestMenu(currentTest);
        currentTest = testMenu;

        testMenu->RegisterTest<GLObject::TestClearColor>("Clear Color");
        testMenu->RegisterTest<GLObject::TestTriangle2D>("2D Plane");
        testMenu->RegisterTest<GLObject::TestTexture2D>("2D Texture");
        testMenu->RegisterTest<GLObject::TestPyramid3D>("3D Pyramid");

        
        //# Camera Initialisation
        //#######################
        CameraHandler cameraHandler;


        while (!glfwWindowShouldClose(window))
        {
            r.Clear();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();

            //# Test window
            //#############

            {
                if (currentTest)
                {
                    currentTest->OnUpdate(0.0f);
                    currentTest->OnRender();


                    ImGui::NewFrame();
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

            glfwPollEvents();
        }

        delete currentTest;
        if (currentTest != testMenu)
            delete testMenu;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

