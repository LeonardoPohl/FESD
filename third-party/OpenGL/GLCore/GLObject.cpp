#include "imgui/imgui.h"
#include "GLObject.h"

namespace GLObject
{
	TestMenu::TestMenu(GLObject *&currentTestPointer)
		: m_CurrentTest(currentTestPointer) { }

	TestMenu::TestMenu(Camera *cam, GLObject *&currentTestPointer)
		: m_CurrentTest(currentTestPointer)		
	{
		camera = cam;
	}


	void TestMenu::OnImGuiRender()
	{
		for (auto const &test : m_Tests)
		{
			if (ImGui::Button(test.first.c_str()))
			{
				m_CurrentTest = test.second(camera);
			}
		}
	}
}