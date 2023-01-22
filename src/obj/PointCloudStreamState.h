#pragma once
#include <array>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

struct PointCloudStreamState
{
	enum State
	{
		STREAM,
		IDLE,
		AVERAGING,
		NORMALS,
		ICP
	};

	static const int m_StateCount = 5;
	
	const std::array<const char *, m_StateCount> m_StateNames{ "Stream", "Idle", "Averaging", "Show Normals", "ICP"};

	State m_State{ STREAM };
	int m_StateElem{ 0 };

	bool operator==(State state) const
	{
		return this->m_State == state;
	}

	void setState(State state)
	{
		m_State = state;

		if (state == STREAM)
			m_StateElem = 0;
		else if (state == IDLE)
			m_StateElem = 1;
		else if (state == AVERAGING)
			m_StateElem = 2;
		else if (state == NORMALS)
			m_StateElem = 3;
		else if (state == ICP)
			m_StateElem = 4;
		
	}

	void showState()
	{
		ImGui::BeginDisabled();
		const char *state_name = m_StateNames[m_StateElem];
		if (ImGui::SliderInt("State", &m_StateElem, 0, m_StateCount - 1, state_name))
			m_State = static_cast<State>(m_StateElem);
		ImGui::EndDisabled();
	}
};
