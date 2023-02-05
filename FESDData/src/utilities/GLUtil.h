#pragma once

#include <memory>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>

#include <GLCore/Renderer.h>
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

	static void setFlags() {
		GLCall(glPointSize(1.5f));
		GLCall(glEnable(GL_BLEND));
		GLCall(glEnable(GL_DEPTH_TEST));
		GLCall(glDepthFunc(GL_LESS));
		GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	}
};
