#pragma once
#include <array>
#include <memory>
#include <unordered_map>

#include <glm/glm.hpp>
#include <GLCore/GLObject.h>
#include <GLCore/Renderer.h>

#include "cameras/DepthCamera.h"
#include "Plane.h"
#include "Cell.h"
#ifndef POINT
#include "Point.h"
#endif
#include "BoundingBox.h"
#include "PointCloudStreamState.h"
#include "utilities/GLUtil.h"

namespace GLObject
{
	class PointCloud : public GLObject
	{
	public:
		PointCloud(DepthCamera *depthCamera, const Camera *cam = nullptr, Renderer *renderer = nullptr, float metersPerUnit = 0.0f);
		
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
			m_NonPlanarpCells.clear();

			m_CellsAssigned = false;
			m_ShowAverageNormals = false;
			m_NormalsCalculated = false;
		}

		void streamDepth(int i, const int16_t *depth);
		void startNormalCalculation();
		void calculateNormals(int i);
		void startCellAssignment();
		void assignCells(int i);
		void startCellCalculation();
		void calculateCells(int i);
		void doPlaneSegmentation();

		PointCloudStreamState m_State{ };

		DepthCamera *mp_DepthCamera;

		Point *m_Points; 
		Point::Vertex *m_Vertices;

		GLUtil m_GLUtil{};

		std::default_random_engine m_Generator;
		std::unique_ptr<std::uniform_int_distribution<int>> m_PointDistribution{};
		std::unique_ptr<std::uniform_int_distribution<int>> m_ColorDistribution{};

		int m_NumCellDevisions{ 200 };
		int m_NumElements{ 0 };
		int m_StreamWidth{ 0 };
		int m_StreamHeight{ 0 };

		std::unordered_map<Cell*, glm::vec3> m_ColorBypCell;
		std::unordered_map<std::string, Cell*> m_pCellByKey;

		std::vector<Cell *> m_PlanarpCells;
		std::vector<Cell *> m_NonPlanarpCells;

		BoundingBox m_BoundingBox{ };
		glm::vec3 m_CellSize{ };

		float m_PlanarThreshold{ 0.004f };

		bool m_CellsAssigned{ false };
		bool m_ShowAverageNormals{ false };
		bool m_NormalsCalculated{ false };

		float m_MetersPerUnit{ 0.0f };
	};
};
