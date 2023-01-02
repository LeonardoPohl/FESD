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

struct GLUtil
{
	Renderer* mp_Renderer;
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
			ImGui::SliderFloat("Scale", &m_Scale, 0.001f, 10.0f);
		}
	}
};
