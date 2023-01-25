#pragma once
#include <array>
#include <memory>
#include <unordered_map>
#include <type_traits>

#include <glm/glm.hpp>
#include <GLCore/GLObject.h>
#include <GLCore/Renderer.h>

// pcl base
#include <pcl/io/pcd_io.h>
#include <pcl/pcl_base.h>
#include <pcl/point_types.h>

// Estimating Keypoints
//#include <pcl/keypoints/narf_keypoint.h>
#include <pcl/keypoints/iss_3d.h>
#include <pcl/keypoints/sift_keypoint.h>

// Describing keypoints - Feature descriptors
#include <pcl/features/fpfh.h>
#include <pcl/features/normal_3d.h>
#include <pcl/features/our_cvfh.h>
#include <pcl/features/principal_curvatures.h>
#include <pcl/features/intensity_spin.h>

// Correspondence Estimation
#include <pcl/registration/correspondence_estimation.h>
#include <pcl/registration/correspondence_estimation_backprojection.h>
#include <pcl/registration/correspondence_estimation_normal_shooting.h>

// Correspondence rejection
#include <pcl/registration/correspondence_rejection_sample_consensus.h>
#include <pcl/registration/correspondence_rejection_distance.h>
#include <pcl/registration/correspondence_rejection_features.h>
#include <pcl/registration/correspondence_rejection_poly.h>

// Transformation Estimation
#include <pcl/registration/transformation_estimation_svd.h>

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

		pcl::PointCloud<pcl::PointXYZ>::Ptr m_CloudStatic;
		pcl::PointCloud<pcl::PointXYZ>::Ptr m_CloudDynamic;

		void acquireData();

		float m_LeafSize{ 0.01f };

		void filterData();
		
		bool m_NormalsCalculated{ false };
		pcl::PointCloud<pcl::Normal>::Ptr m_NormalsStatic;
		pcl::PointCloud<pcl::Normal>::Ptr m_NormalsDynamic;

		void computeNormals();

		// SIFT
		bool m_UseSIFT{ true };
		float m_MinScale{ 0.1f };
		int m_NOctaves{ 6 };
		int m_NScalesPerOctave{ 10 };
		float m_MinContrast{ 0.5f };
		pcl::PointCloud<pcl::PointXYZ>::Ptr m_SIFTKPStatic;
		pcl::PointCloud<pcl::PointXYZ>::Ptr m_SIFTKPDynamic;

		void estimateKeyPoints();
		
		// FPFHE
		pcl::PointCloud<pcl::FPFHSignature33>::Ptr m_FPFHsDynamic;
		pcl::PointCloud<pcl::FPFHSignature33>::Ptr m_FPFHsStatic;

		// NormalEstimation
		//pcl::NormalEstimation< pcl::PointXYZ, pcl::PointXYZ > m_NormalEstimation;


		//pcl::OURCVFHEstimation< pcl::PointXYZ, pcl::PointXYZ, pcl::PointXYZ > m_OURCVFHEstimation;
		//pcl::PrincipalCurvaturesEstimation< pcl::PointXYZ, pcl::PointXYZ, pcl::PointXYZ > m_PrincipalCurvatureEstimation;
		//pcl::IntensitySpinEstimation< pcl::PointXYZ, pcl::PointXYZ > m_IntensitySpinEstimation;
		
		void describeKeyPoints();

		/*
		pcl::registration::CorrespondenceEstimation< PointSource, PointTarget, Scalar >
		pcl::registration::CorrespondenceEstimationBackProjection< PointSource, PointTarget, NormalT, Scalar >
		pcl::registration::CorrespondenceEstimationNormalShooting< PointSource, PointTarget, NormalT, Scalar >
		*/
		void findCorrespondence();

		/*
		pcl::registration::CorrespondenceRejectorSampleConsensus< PointT >
		pcl::registration::CorrespondenceRejectorDistance
		pcl::registration::CorrespondenceRejectorFeatures::FeatureContainer< FeatureT >
		pcl::registration::CorrespondenceRejectorPoly< SourceT, TargetT >
		*/
		void rejectCorrespondence();

		//void alignPointcloudsNDT();
		//void alignPointclouds();

		Logger::Logger* mp_Logger;

		PointCloudStreamState m_State{ };

		std::vector<DepthCamera*> m_DepthCameras;
		std::vector<glm::mat4> m_MVPS{};
		const int m_CameraCount{ };

		std::vector<std::shared_ptr<Point[]>> m_Points;

		GLUtil m_GLUtil{ };

		glm::vec3 m_Rotation{ 0 };
		glm::vec3 m_Translation{ 0 };
		float m_Scale{ 1.0f };

		int m_NumElementsTotal{ 0 };
		std::vector<int> m_NumElements;
		std::vector<int> m_ElementOffset;
		std::vector<int> m_StreamWidths;
		std::vector<int> m_StreamHeights;

		std::vector<BoundingBox> m_BoundingBoxes{ };
		std::vector<glm::vec3> m_CellSizes{ };

		bool m_AlignmentMode{ false };
		bool m_ICPInitialised{ false };
		bool m_IsAligned{ false };

		float m_TransformationEpsilon{ 0.01 };
		float m_StepSize{ 0.1 };
		float m_Resolution{ 0.5 };
		int m_MaxIterations{ 50 };
	};
};
