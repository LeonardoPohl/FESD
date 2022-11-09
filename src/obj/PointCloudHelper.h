#pragma once
#include <memory>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>

#include <GLCore/Renderer.h>
#include <GLCore/Texture.h>
#include <GLCore/VertexBuffer.h>
#include <GLCore/VertexBufferLayout.h>

struct PointCloudStreamState
{
	enum State
	{
		STREAM,
		IDLE,
		NORMALS,
		CELLS,
		CALC_CELLS
	};

	static const int m_StateCount = 6;
	
	const std::array<const char *, m_StateCount> m_StateNames{ "Stream", "Idle", "Show Normals", "Show Cells", "Calculate Cells" };

	State m_State{ STREAM };
	int m_StateElem{ 0 };

	bool operator==(State state) const
	{
		return this->m_State == state;
	}

	void setState(State state)
	{
		if (state == STREAM)
		{
			m_State = PointCloudStreamState::STREAM;
			m_StateElem = 0;
		}
		else if (state == IDLE)
		{
			m_State = PointCloudStreamState::IDLE;
			m_StateElem = 1;
		}
		else if (state == NORMALS)
		{
			m_State = NORMALS;
			m_StateElem = 2;
		}
		else if (state == CELLS)
		{
			m_State = CELLS;
			m_StateElem = 3;
		}
		else if (state == CALC_CELLS)
		{
			m_State = CALC_CELLS;
			m_StateElem = 4;
		}
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

struct GLUtil
{
	Renderer *mp_Renderer;
	std::unique_ptr<VertexArray> m_VAO;
	std::unique_ptr<IndexBuffer> m_IndexBuffer;
	std::unique_ptr<Shader> m_Shader;
	std::unique_ptr<VertexBuffer> m_VB;
	std::unique_ptr<VertexBufferLayout> m_VBL;

	float m_RotationFactor{ 0 };
	glm::vec3 m_Rotation{ 0.0f, 1.0f, 0.0f };
	glm::vec3 m_Translation{ 0.f, 0.f, 0.f };
	glm::vec3 m_ModelTranslation{ 0.0f };

	float m_Scale{ 1.0f };
	float m_DepthScale{ 5.0f };

	void manipulateTranslation()
	{
		if (ImGui::CollapsingHeader("Translation"))
		{
			ImGui::SliderAngle("Rotation Factor", &m_RotationFactor);
			ImGui::SliderFloat3("Rotation", &m_Rotation.x, -1.0f, 1.0f);

			ImGui::SliderFloat3("Translation", &m_Translation.x, -2.0f, 2.0f);
		}

		if (ImGui::CollapsingHeader("Scale"))
		{
			ImGui::SliderFloat("Depth Scale", &m_DepthScale, 0.001f, 30.0f);
			ImGui::SliderFloat("Scale", &m_Scale, 0.001f, 10.0f);
		}
	}
};
