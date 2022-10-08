#include "TestClearColor.h"

#include <GL/glew.h>
#include <GLCore/GLErrorManager.h>
#include <imgui.h>

namespace GLObject
{

	TestClearColor::TestClearColor() 
	{
		GLCall(glGetFloatv(GL_COLOR_CLEAR_VALUE, m_ClearColor));
	}

	TestClearColor::~TestClearColor() {}

	void TestClearColor::OnUpdate(float deltaTime) {}

	void TestClearColor::OnRender()
	{
		GLCall(glClearColor(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]))
		GLCall(glClear(GL_COLOR_BUFFER_BIT))
	}

	void TestClearColor::OnImGuiRender()
	{
		ImGui::ColorEdit4("Clear Color", m_ClearColor);
	}
}