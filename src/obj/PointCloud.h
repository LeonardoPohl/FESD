#pragma once
#include <array>
#include <memory>
#include <unordered_map>
#include <type_traits>

#include <glm/glm.hpp>
#include <GLCore/GLObject.h>
#include <GLCore/Renderer.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/registration/icp.h>
#include <pcl/registration/icp_nl.h>
#include <pcl/registration/ndt.h>
#include <pcl/filters/approximate_voxel_grid.h>

#include "cameras/DepthCamera.h"
#include "Logger.h"
#include "Point.h"
#include "BoundingBox.h"
#include "PointCloudStreamState.h"
#include "utilities/GLUtil.h"

namespace GLObject
{
	class PointCloud : public GLObject
	{
	public:
		// Constructor
		PointCloud(std::vector<DepthCamera*> depthCameras, const Camera *cam, Logger::Logger* logger, Renderer *renderer);
		
		// Updates
		void OnUpdate() override;
		void OnRender() override;
		void OnImGuiRender() override;
		void manipulateTranslation();

		glm::vec3 getRotation();
		glm::vec3 getTranslation();
	private:
		void pauseStream();
		void resumeStream();

		void streamDepth(int cam_index, const int16_t* depth);
		void calculateNormals(int cam_index);
		void alignPointcloudsNDT();
		void alignPointclouds();

		Logger::Logger* mp_Logger;

		PointCloudStreamState m_State{ };

		std::vector<DepthCamera*> m_DepthCameras;
		std::vector<glm::mat4> m_MVPS{};
		const int m_CameraCount{ };

		std::vector<std::shared_ptr<Point[]>> m_Points;

		GLUtil m_GLUtil{ };

		glm::vec3 m_Rotation;
		glm::vec3 m_Translation;
		float m_Scale{ 1.0f };

		int m_NumElementsTotal{ 0 };
		std::vector<int> m_NumElements;
		std::vector<int> m_ElementOffset;
		std::vector<int> m_StreamWidths;
		std::vector<int> m_StreamHeights;

		std::vector<BoundingBox> m_BoundingBoxes{ };
		std::vector<glm::vec3> m_CellSizes{ };

		int m_AveragingCount{ 0 };
		bool m_NormalsCalculated{ false };

		pcl::PointCloud<pcl::PointXYZ>::Ptr m_CloudIn;
		pcl::PointCloud<pcl::PointXYZ>::Ptr m_CloudOut;
		pcl::IterativeClosestPoint<pcl::PointXYZ, pcl::PointXYZ> m_ICP{ };
		bool m_AlignmentMode{ false };
		bool m_ICPInitialised{ false };
		bool m_IsAligned{ false };

		float m_TransformationEpsilon{ 0.01 };
		float m_StepSize{ 0.1 };
		float m_Resolution{ 1.0 };
		int m_MaxIterations{ 50 };
	};
};
