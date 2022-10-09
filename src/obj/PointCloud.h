#pragma once

#include <GLCore/GLObject.h>

#include <GLCore/Renderer.h>
#include <GLCore/Texture.h>
#include <GLCore/VertexBuffer.h>
#include <GLCore/VertexBufferLayout.h>

#include <array>
#include <memory>

class Point
{
public:
	std::array<int, 2>  Position;

	struct Vertex
	{
		std::array<float, 3> Position;
		std::array<float, 3> Color;

		Vertex(std::array<float, 3> position,
			   std::array<float, 3> color) 
			: Position(position), Color(color) {}
	};

	Point(int x, int y) : Position{x, y}{ }

	std::array<float, 3> getColorFromDepth(unsigned int depth)
	{
		// TODO: Implement
		return { 1.0f, 1.0f, 1.0f };
	}

	std::array<Vertex, 5> getVertexArray(unsigned int depth)
	{
		std::array<float, 3> Color = getColorFromDepth(depth);

		Vertex v0 ({ -0.5f + Position[0], 0.0f + Position[1],  0.5f + depth }, Color);
		Vertex v1 ({ -0.5f + Position[0], 0.0f + Position[1], -0.5f + depth }, Color);
		Vertex v2 ({  0.5f + Position[0], 0.0f + Position[1], -0.5f + depth }, Color);
		Vertex v3 ({  0.5f + Position[0], 0.0f + Position[1],  0.5f + depth }, Color);
		Vertex v4 ({  0.0f + Position[0], 0.8f + Position[1],  0.0f + depth }, Color);

		return { v0, v1, v2, v3, v4 };
	}

};

namespace GLObject
{
	class PointCloud : public GLObject
	{
	public:
		PointCloud() =default;


		void OnRender() override;
		void OnImGuiRender() override;
	private:
		float m_Color[4];




		std::unique_ptr<VertexArray> m_VAO;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
		std::unique_ptr<Shader> m_Shader;
		std::unique_ptr<Texture> m_Texture;
		std::unique_ptr<VertexBuffer> m_VB;
		std::unique_ptr<VertexBufferLayout> m_VBL;

		glm::mat4 m_View;
		glm::mat4 m_Proj;

		float m_RotationFactor;
		glm::vec3 m_Rotation;
		glm::vec3 m_Translation;

		float m_Scale;
	};
}