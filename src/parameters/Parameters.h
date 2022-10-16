#pragma once

#include <imgui.h>
#include <cameras/DepthCamera.h>
#include <vector>

namespace Params
{
	// TODO: Add save capability
	// TODO: Investigate if this should be camera specific

	class Parameters {
	public:
		Parameters() = default;
		virtual void displayParameters() = 0;
	};

	class GlobalParameters : public Parameters {
	public:
		GlobalParameters(std::vector<DepthCamera*>* depth_cameras) : depth_cameras(depth_cameras) {};
		void displayParameters() override;

		float sphere_radius;
		bool display_edges = false;
		bool walking_average = false;
		bool calculate_surface_normals = false;
		std::vector<DepthCamera*>* depth_cameras;
	};

	class SphereDetectionParameters : public Parameters {
	public:
		void displayParameters() override;

		float sphere_radius{ 50 };

		float param1{ 10 };
		float param2{ 10 };
		int min_radius{ 0 };
		int max_radius{ 10 };

		int block_size{};

		int current_adaptive_threshold = 0;
		int current_threshold = 0;
		/*
		std::vector<cv::AdaptiveThresholdTypes> adapriveThresholdTypes{
			cv::AdaptiveThresholdTypes::ADAPTIVE_THRESH_MEAN_C ,
			cv::AdaptiveThresholdTypes::ADAPTIVE_THRESH_GAUSSIAN_C
		};

		std::vector<cv::ThresholdTypes> thresholdTypes{
			cv::ThresholdTypes::THRESH_BINARY,
			cv::ThresholdTypes::THRESH_BINARY_INV,
			cv::ThresholdTypes::THRESH_TRUNC,
			cv::ThresholdTypes::THRESH_BINARY_INV,
			cv::ThresholdTypes::THRESH_TOZERO,
			cv::ThresholdTypes::THRESH_TOZERO_INV,
			cv::ThresholdTypes::THRESH_MASK,
			cv::ThresholdTypes::THRESH_OTSU,
			cv::ThresholdTypes::THRESH_TRIANGLE
		};

		cv::AdaptiveThresholdTypes adapriveThresholdType{ cv::AdaptiveThresholdTypes::ADAPTIVE_THRESH_MEAN_C };
		cv::ThresholdTypes thresholdType{ cv::ThresholdTypes::THRESH_BINARY };*/
	};

	class NormalParameters : public Parameters {
	public:
		void displayParameters() override;

		float upnessFilter{ 1 };
		float edgeCutoff{ 10 };
		int whatsUp;
		int num_samples;
		bool blur;
	};
}