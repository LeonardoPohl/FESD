#pragma once

#include <GLCore/GLObject.h>

#include <GLCore/Renderer.h>
#include <GLCore/Texture.h>
#include <GLCore/VertexBuffer.h>
#include <GLCore/VertexBufferLayout.h>

#include <memory>

namespace GLObject
{
	class TestTexture2D : public GLObject
	{
	public:
		TestTexture2D(const Camera *cam = nullptr);

		void OnRender() override;
		void OnImGuiRender() override;
	private:
		glm::vec3 m_TranslationA{ 200, 200, 0 };
		glm::vec3 m_TranslationB{ 400, 200, 0 };
		std::unique_ptr<VertexArray> m_VAO;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
		std::unique_ptr<Shader> m_Shader;
		std::unique_ptr<Texture> m_Texture;
		std::unique_ptr<VertexBuffer> m_VB;
		std::unique_ptr<VertexBufferLayout> m_VBL;

		glm::mat4 m_View = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));
		glm::mat4 m_Proj = glm::ortho(0.0f, 960.0f, 0.0f, 540.0f, -1.0f, 1.0f);
	};
}