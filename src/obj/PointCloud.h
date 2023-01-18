#pragma once
#include <array>
#include <memory>
#include <unordered_map>
#include <type_traits>

#include <glm/glm.hpp>
#include <GLCore/GLObject.h>
#include <GLCore/Renderer.h>
#include <nanoflann.hpp>

#include "cameras/DepthCamera.h"
#include "Point.h"
#include "BoundingBox.h"
#include "PointCloudStreamState.h"
#include "utilities/GLUtil.h"
#include "KDTree.h"

namespace GLObject
{
	class PointCloud : public GLObject
	{
	public:
		// Constructor
		PointCloud(std::vector<DepthCamera*> depthCameras, const Camera *cam = nullptr, Renderer *renderer = nullptr);
		
		// Updates
		void OnUpdate() override;
		void OnRender() override;
		void OnImGuiRender() override;
		void manipulateTranslation();

		glm::vec3 getRotation(int cameraId);
		glm::vec3 getTranslation(int cameraId);
	private:
		void pauseStream();
		void resumeStream();

		void streamDepth(int i, int cam_index, const int16_t* depth);
		void startNormalCalculation();
		void calculateNormals(int i, int cam_index);
		void alignPointclouds();

		PointCloudStreamState m_State{ };

		std::vector<DepthCamera*> m_DepthCameras;
		std::vector<glm::mat4> m_MVPS{};
		const int m_CameraCount{ };

		std::vector<Point *> m_Points; 
		std::vector<Point::Vertex *> m_Vertices;

		GLUtil m_GLUtil{ };

		glm::vec3 m_Rotation;
		glm::vec3 m_Translation;
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

		std::vector<BoundingBox> m_BoundingBoxes{ };
		std::vector<glm::vec3> m_CellSizes{ };
		
		// For now only works if there are exactly 2 cameras and no more
		KDTree::Tree *m_KDTree;

		float m_PlanarThreshold{ 0.004f };

		bool m_CellsAssigned{ false };
		bool m_ShowAverageNormals{ false };
		bool m_NormalsCalculated{ false };
		bool m_AlignmentMode{ false };
		bool m_KDTreeBuilt{ false };
	};
};
