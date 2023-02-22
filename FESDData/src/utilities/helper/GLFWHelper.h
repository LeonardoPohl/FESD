#pragma once

#include <opencv2/opencv.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

enum STATUS;

GLFWwindow *InitialiseGLFWWindow(STATUS &status);
GLuint matToTexture(const cv::Mat& mat, GLenum minFilter, GLenum magFilter, GLenum wrapFilter);
