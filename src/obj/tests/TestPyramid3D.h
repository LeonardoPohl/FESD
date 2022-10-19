#pragma once

#include <GLCore/GLObject.h>

#include <GLCore/Renderer.h>
#include <GLCore/Texture.h>
#include <GLCore/VertexBuffer.h>
#include <GLCore/VertexBufferLayout.h>

#include <memory>

namespace GLObject
{
	class TestPyramid3D : public GLObject
	{
	public:
		TestPyramid3D(const Camera *cam = nullptr);

		void OnRender() override;
		void OnImGuiRender() override;
	private:
		float m_Color[4]{ 0.2f, 0.3f, 0.8f, 1.0f };

		glm::vec2 m_LeftRightOrtho;
		glm::vec2 m_BottomTopOrtho;
		glm::vec2 m_NearFarOrtho;

		glm::vec3 m_ModelTranslation;
		glm::vec3 m_ViewTranslation;

		std::unique_ptr<VertexArray> m_VAO;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
		std::unique_ptr<Shader> m_Shader;
		std::unique_ptr<Texture> m_Texture;
		std::unique_ptr<VertexBuffer> m_VB;
		std::unique_ptr<VertexBufferLayout> m_VBL;

		glm::mat4 m_View = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));
		glm::mat4 m_Proj;

		float m_RotationFactor{0};
		glm::vec3 m_Rotation{ 0.0f, 1.0f, 0.0f };
		glm::vec3 m_Translation{ 0.0f, 1.0f, 0.0f };

		float m_Scale{0.5f};
	};
}