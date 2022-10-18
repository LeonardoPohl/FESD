#include "imgui/imgui.h"
#include "GLObject.h"

namespace GLObject
{
	void TestMenu::OnStart(Arguments *args)
	{
		auto testMenuArgs = (TestMenuArguments *)args;

		m_CurrentTest = testMenuArgs->currentTestPointer;
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