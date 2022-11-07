#pragma once

#include <GLCore/GLObject.h>

#include <GLCore/Renderer.h>
#include <GLCore/Texture.h>
#include <GLCore/VertexBuffer.h>
#include <GLCore/VertexBufferLayout.h>

#include <array>
#include <memory>
#include <cameras/DepthCamera.h>

#include <unordered_map>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "utilities/Point.h"
#include "utilities/Plane.h"
#include "utilities/Cell.h"

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

		void pauseStream()
		{
			state_elem = 1;
			state = IDLE;
		}

		void resumeStream()
		{
			state_elem = 0;
			state = STREAM;

			for (auto key : cellByKey)
				delete key.second;

			cellByKey.clear();
			colorByCell.clear();
		}

		void streamDepth();
		void calculateNormals();
		void assignCells();
		void calculateCells();

		static const int StateCount = 6;
		static const char *StateNames[];

		enum State
		{
			STREAM,
			IDLE,
			NORMALS,
			CELLS,
			CALC_CELLS
		};

		State state{ STREAM };
		int state_elem{ 0 };

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
		glm::vec3 m_Translation { 0.f, 0.f, 0.f };
		glm::vec3 m_ModelTranslation{ 0.0f };

		float m_Scale {1.0f};
		float m_Depth_Scale {5.0f};
		/*
		float m_MaxDepth {0.0f};
		float m_DistanceThreshold{1.0f};
		int m_PointCountThreshold{ 150000 };
		*/
		std::default_random_engine m_Generator;
		std::unique_ptr<std::uniform_int_distribution<int>> m_Distribution{};

		Point::CMAP cmap{ Point::CMAP::VIRIDIS };
		int cmap_elem{ 0 };

		std::vector<std::pair<Plane, int>> pointCountByPlane;
		int maxPointCount{ 0 };

		int m_OctTreeDevisions{ 200 };
		int m_NumElements{ 0 };
		int m_StreamWidth{ 0 };
		int m_StreamHeight{ 0 };

		std::unordered_map<Cell*, glm::vec3> colorByCell;
		std::unordered_map<std::string, Cell*> cellByKey;

		// TODO low prio: draw bounding box
		glm::vec3 m_MinBoundingPoint{};
		glm::vec3 m_MaxBoundingPoint{};

		bool m_CellsAssigned{ false };
		bool m_ShowAverageNormals{ false };
		bool m_NormalsCalculated{ false };
	};
};
