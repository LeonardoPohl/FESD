#pragma once

struct GLFWwindow;

class ImGuiHelper
{
public:
	static void initImGui(GLFWwindow *window);
	static void beginFrame();
	static void endFrame();
	static void terminateImGui();

	static void HelpMarker(const char* desc);
};