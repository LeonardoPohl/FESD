#pragma once

#include <opencv2/opencv.hpp>
struct GLFWwindow;

class ImGuiHelper
{
public:
	static void initImGui(GLFWwindow *window);
	static void beginFrame();
	static void endFrame();
	static void terminateImGui();

	static void HelpMarker(const char* desc);

	static void showImage(cv::Mat im);
};