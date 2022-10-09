#include "imgui/imgui.h"
#include "GLObject.h"

namespace GLObject
{
	TestMenu::TestMenu(GLObject *&currentTestPointer)
		: m_CurrentTest(currentTestPointer)
	{
	}

	void TestMenu::OnImGuiRender()
	{
		for (auto &test : m_Tests)
		{
			if (ImGui::Button(test.first.c_str()))
			{
				m_CurrentTest = test.second();
			}
		}
	}
}