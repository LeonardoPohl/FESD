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
	class TestBatchPoints : public GLObject
	{
	public:
		TestBatchPoints();

		void OnRender() override;
		void OnImGuiRender() override;
		bool m_RenderPointCloud = false;
	private:
		std::array<float, 4> m_Color{ 0.2f, 0.3f, 0.8f, 1.0f };

		Point *m_Points;
		Point::Vertex *m_Vertices;

		std::unique_ptr<VertexArray> m_VAO;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
		std::unique_ptr<Shader> m_Shader;
		std::unique_ptr<VertexBuffer> m_VB;
		std::unique_ptr<VertexBufferLayout> m_VBL;

		// TODO: Replace with camera
		glm::mat4 m_View{ glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, -2.0f)) };
		glm::mat4 m_Proj;

		// TODO: Very Low Prio current is fine: Replace with Transformation gizmo
		float m_RotationFactor{ 0 };
		glm::vec3 m_Rotation{ 0.0f, 1.0f, 0.0f };
		glm::vec3 m_Translation{ 0.0f, 1.0f, 0.0f };

		float m_Scale{ 0.5f };
		float m_ClearColor[4];
	};
};
