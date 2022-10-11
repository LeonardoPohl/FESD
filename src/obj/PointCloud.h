#pragma once

#include <GLCore/GLObject.h>

#include <GLCore/Renderer.h>
#include <GLCore/Texture.h>
#include <GLCore/VertexBuffer.h>
#include <GLCore/VertexBufferLayout.h>

#include <array>
#include <memory>
#include <cameras/DepthCamera.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

class Point
{
public:
	struct Vertex
	{
		std::array<float, 3> Position;
		std::array<float, 3> Color;

		Vertex() :Position{0.0f,0.0f,0.0f}, Color{ 0.0f,0.0f,0.0f } { };
		Vertex(std::array<float, 3> position,
			   std::array<float, 3> color)
			: Position(position), Color(color)
		{
		}
	};

	static const int VertexCount = 5;
	static const int IndexCount = 3 * 6;
	std::array<float, 2> Position;
	float Depth{ 0 };
	std::array<Vertex, VertexCount> Vertices;

	Point() : Position{ 0, 0 }
	{
	}

	inline std::array<float, 3> getColorFromDepth(float depth)
	{
		// TODO: Implement
		return { 1.0f, 0.0f, 1.0f };
	}

	inline void updateDepth(float depth)
	{
		this->Depth = depth;
		Vertices[0].Position[2] =  5.0f + depth;
		Vertices[1].Position[2] = -5.0f + depth;
		Vertices[2].Position[2] = -5.0f + depth;
		Vertices[3].Position[2] =  5.0f + depth;
		Vertices[4].Position[2] =  0.0f + depth;
	}

	inline int *getIndices(int i)
	{
		std::array<int, 3 * 6> indices
		{
			0, 1, 2,
			0, 2, 3,
			0, 1, 4,
			1, 2, 4,
			2, 3, 4,
			3, 0, 4
		};

		for (auto index : indices)
		{
			index += i * VertexCount;
		}

		return &indices[0];
	}

	inline void setVertexArray()
	{
		std::array<float, 3> Color = getColorFromDepth(Depth);

		/*
		   4
		   ^
		  /|\
		 /0| \
		/.-|-.\
	   1'-.|.-'3
		   2
		*/

		Vertices[0] = { { -5.0f + Position[0], 0.0f + Position[1],  5.0f }, Color };
		Vertices[1] = { { -5.0f + Position[0], 0.0f + Position[1], -5.0f }, Color };
		Vertices[2] = { {  5.0f + Position[0], 0.0f + Position[1], -5.0f }, Color };
		Vertices[3] = { {  5.0f + Position[0], 0.0f + Position[1],  5.0f }, Color };
		Vertices[4] = { {  0.0f + Position[0], 5.0f + Position[1],  0.0f }, Color };
	}

};

namespace GLObject
{
	class PointCloud : public GLObject
	{
	public:
		PointCloud()
		{
			std::cout << "Do not initialise the point cloud without a depth camera.";
			return;
		}
		PointCloud(DepthCamera* depthCamera);

		void OnRender() override;
		void OnImGuiRender() override;
		bool m_RenderPointCloud = false;
	private:
		std::array<float,4> m_Color{ 0.2f, 0.3f, 0.8f, 1.0f };

		DepthCamera *m_DepthCamera;

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
		float m_RotationFactor {0};
		glm::vec3 m_Rotation { 0.0f, 1.0f, 0.0f };
		glm::vec3 m_Translation { 0.0f, 1.0f, 0.0f };

		float m_Scale {0.5f};

	};
};
