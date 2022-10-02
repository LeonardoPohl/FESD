#pragma once
#include <opencv2/imgproc.hpp>
#include <imgui.h>
#include <DepthCamera.h>

namespace Params
{
	// TODO Implement all parameters
	/*
	@param src Source 8 - bit single - channel image.
	@param dst Destination image of the same size and the same type as src.
	@param maxValue Non - zero value assigned to the pixels for which the condition is satisfied
	@param adaptiveMethod Adaptive thresholding algorithm to use, see #AdaptiveThresholdTypes.
	The #BORDER_REPLICATE | #BORDER_ISOLATED is used to process boundaries.
	@param thresholdType Thresholding type that must be either #THRESH_BINARY or #THRESH_BINARY_INV,
	see #ThresholdTypes.
	@param blockSize Size of a pixel neighborhood that is used to calculate a threshold value for the
	pixel : 3, 5, 7, and so on.
	@param C Constant subtracted from the mean or weighted mean(see the details below).Normally, it
	is positive but may be zero or negative as well.
	*/

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
		cv::ThresholdTypes thresholdType{ cv::ThresholdTypes::THRESH_BINARY };
	};

	class NormalParameters : public Parameters {
	public:
		void displayParameters() override;

		float upnessFilter{ 1 };
		float edgeCutoff{ 10 };
		int whatsUp;
		int num_samples;
	};
}