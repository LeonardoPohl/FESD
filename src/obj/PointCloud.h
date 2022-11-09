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
#include "utilities/Utilities.h"

#include "PointCloudHelper.h"

namespace GLObject
{
	class PointCloud : public GLObject
	{
	public:
		PointCloud(DepthCamera *depthCamera, const Camera *cam = nullptr, Renderer *renderer = nullptr);
		
		void OnUpdate() override;
		void OnRender() override;
		void OnImGuiRender() override;

	private:
		void pauseStream()
		{
			m_State.setState(PointCloudStreamState::IDLE);
		}

		void resumeStream()
		{
			m_State.setState(PointCloudStreamState::STREAM);

			for (auto key : m_pCellByKey)
				delete key.second;

			m_pCellByKey.clear();
			m_ColorBypCell.clear();
			m_PlanarpCells.clear();
			m_NonPlanarpPoints.clear();
		}

		void streamDepth(int i, const int16_t *depth);
		void startNormalCalculation();
		void calculateNormals(int i);
		void startCellAssignment();
		void assignCells(int i);
		void startCellCalculation();
		void calculateCells(int i);

		PointCloudStreamState m_State{ };

		DepthCamera *mp_DepthCamera;

		Point *m_Points; 
		Point::Vertex *m_Vertices;

		GLUtil m_GLUtil{};

		std::default_random_engine m_Generator;
		std::unique_ptr<std::uniform_int_distribution<int>> m_PointDistribution{};
		std::unique_ptr<std::uniform_int_distribution<int>> m_ColorDistribution{};

		Point::CMAP m_CMAP{ Point::CMAP::VIRIDIS };
		int m_CMAPElem{ 0 };

		std::vector<std::pair<Plane, int>> m_PointCountByPlane;
		int m_MaxPointCount{ 0 };

		int m_OctTreeDevisions{ 200 };
		int m_NumElements{ 0 };
		int m_StreamWidth{ 0 };
		int m_StreamHeight{ 0 };

		std::unordered_map<Cell*, glm::vec3> m_ColorBypCell;
		std::unordered_map<std::string, Cell*> m_pCellByKey;

		std::vector<Cell *> m_PlanarpCells;
		std::vector<Point *> m_NonPlanarpPoints;

		BoundingBox m_BoundingBox{ };
		glm::vec3 m_CellSize{ };

		float m_PlanarThreshold{ 0.004f };

		bool m_CellsAssigned{ false };
		bool m_ShowAverageNormals{ false };
		bool m_NormalsCalculated{ false };
	};
};
