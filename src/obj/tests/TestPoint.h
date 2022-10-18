#pragma once

#include <GLCore/GLObject.h>

#include <GLCore/Renderer.h>
#include <GLCore/Texture.h>
#include <GLCore/VertexBuffer.h>
#include <GLCore/VertexBufferLayout.h>

#include <array>
#include <memory>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "utilities/Point.h"

namespace GLObject
{
	class TestPoint : public GLObject
	{
	public:
		TestPoint(Arguments *args = nullptr) : GLObject(args) { }
		TestPoint(Camera *cam, Arguments *args = nullptr) : GLObject(cam, args) { }

		void OnStart() override;
		void OnRender() override;
		void OnImGuiRender() override;
	private:
		std::array<float, 2> m_Position;
		float m_Depth;

		unsigned int m_Width;
		unsigned int m_Height;

		float m_Scale {1.0f};
		float m_RotationFactor;
		glm::vec3 m_Rotation {0.0f, 1.0f, 0.0f};

		Point *m_Points;
		Point::Vertex *m_Vertices;

		glm::vec3 m_ModelTranslation {0.0f};

		glm::mat4 m_View{ glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, -2.0f)) };
		glm::mat4 m_Proj;

		std::unique_ptr<VertexArray> m_VAO;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
		std::unique_ptr<Shader> m_Shader;
		std::unique_ptr<VertexBuffer> m_VB;
		std::unique_ptr<VertexBufferLayout> m_VBL;
	};
}