#include "GLFWHelper.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <GLCore/GLErrorManager.h>

#include "Consts.h"

GLFWwindow *InitialiseGLFWWindow(STATUS &status)
{
    GLFWwindow *window;
    
    status = OK;
    
    /* Initialize the library */
    if (!glfwInit())
        status = ERR;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WindowName.c_str(), nullptr, nullptr);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!window)
    {
        glfwTerminate();
        status = ERR;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK)
    {
        std::cout << "Error!" << std::endl;
    }

    std::cout << glGetString(GL_VERSION) << std::endl;
    GLCall(glEnable(GL_BLEND));
    GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    GLCall(glClearColor(.15f, .15f, .15f, 1.f));
    
    return window;
}
