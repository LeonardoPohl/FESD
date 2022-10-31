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
#include "utilities/Point.h"

namespace GLObject
{
	class PointCloud : public GLObject
	{
	public:
		PointCloud(DepthCamera *depthCamera, const Camera *cam = nullptr);

		void OnUpdate() override;
		void OnRender() override;
		void OnImGuiRender() override;
	private:
		DepthCamera *m_DepthCamera;

		Point *m_Points; 
		Point::Vertex *m_Vertices; 

		std::unique_ptr<VertexArray> m_VAO;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
		std::unique_ptr<Shader> m_Shader;
		std::unique_ptr<VertexBuffer> m_VB;
		std::unique_ptr<VertexBufferLayout> m_VBL;

		float m_RotationFactor{ 0 };
		glm::vec3 m_Rotation{ 0.0f, 1.0f, 0.0f };
		glm::vec3 m_Translation { 0.f, 0.f, -5.f };
		glm::vec3 m_ModelTranslation{ 0.0f };

		float m_Scale {1.0f};
		float m_Depth_Scale {5.0f};
		float m_MaxDepth {0.0f};

		bool doUpdate{ true };

		Point::CMAP cmap{ Point::CMAP::VIRIDIS };
		int cmap_elem{ 0 };
	};
};
