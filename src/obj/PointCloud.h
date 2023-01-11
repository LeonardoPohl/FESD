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
		PointCloud(std::vector<DepthCamera*> depthCameras, const Camera *cam = nullptr, Renderer *renderer = nullptr);
		
		void OnUpdate() override;
		void OnRender() override;
		void OnImGuiRender() override;

	private:
		void pauseStream();
		void resumeStream();

		void streamDepth(int i, int cam_index, const int16_t* depth);
		void startNormalCalculation();
		void calculateNormals(int i, int cam_index);
		void startCellAssignment();
		void assignCells(int i, int cam_index);
		void startCellCalculation();
		void calculateCells(int i, int cam_index);
		void doPlaneSegmentation();
		void manipulateTranslation();

		PointCloudStreamState m_State{ };

		std::vector<DepthCamera*> m_DepthCameras;
		std::vector<glm::mat4> m_MVPS{};
		const int m_CameraCount{ };

		std::vector<Point *> m_Points; 
		std::vector<Point::Vertex *> m_Vertices;

		GLUtil m_GLUtil{ };

		std::vector<glm::vec3> m_Rotation;
		std::vector<glm::vec3> m_Translation;
		float m_Scale{ 1.0f };

		std::default_random_engine m_Generator;
		std::unique_ptr<std::uniform_int_distribution<int>> m_PointDistribution{};
		std::unique_ptr<std::uniform_int_distribution<int>> m_ColorDistribution{};

		int m_NumCellDevisions{ 200 };
		int m_NumElementsTotal{ 0 };
		std::vector<int> m_NumElements;
		std::vector<int> m_ElementOffset;
		std::vector<int> m_StreamWidths;
		std::vector<int> m_StreamHeights;

		std::vector<std::unordered_map<Cell*, glm::vec3>> m_ColorBypCell;
		std::vector<std::unordered_map<std::string, Cell*>> m_pCellByKey;

		std::vector<std::vector<Cell *>> m_PlanarpCells;
		std::vector<std::vector<Cell *>> m_NonPlanarpCells;

		std::vector<BoundingBox> m_BoundingBoxes{ };
		std::vector<glm::vec3> m_CellSizes{ };

		float m_PlanarThreshold{ 0.004f };

		bool m_CellsAssigned{ false };
		bool m_ShowAverageNormals{ false };
		bool m_NormalsCalculated{ false };
	};
};
