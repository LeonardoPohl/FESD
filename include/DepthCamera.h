#pragma once
#include <OpenNI.h>				// Include OpenNI
#include <opencv2/core.hpp>		// Include OpenCV
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <stdexcept>

#include <Circle.h>
#include <WalkingAverage.h>
#include <Parameters.h>

class DepthCamera {
public:
	virtual ~DepthCamera() = default;
	virtual cv::Mat getDepthFrame() = 0;
	virtual cv::Mat getColorFrame() = 0;
	virtual std::string getName() const = 0; 
	virtual bool hasColorStream() = 0;
	virtual cv::Vec3f pixelToPoint(int x, int y, ushort depth) const = 0;

	std::vector<Circle*> detectSpheres(Params::SphereDetectionParameters *params);
	std::vector<Circle*> detectSpheres(cv::Mat frame, Params::SphereDetectionParameters *params);

	void displaySphereTable(cv::Mat depth_frame, cv::Mat edge_frame, Params::SphereDetectionParameters *params, bool display_edges);

	static cv::Mat detectEdges(cv::Mat depth_frame, Params::SphereDetectionParameters *params);
	cv::Mat getWorldFrame(cv::Mat depth_frame);

	cv::Mat calculateSelectedFloor(cv::Mat depth_frame, Params::NormalParameters *params);

	void doUpdate(Params::GlobalParameters *global_params, Params::NormalParameters *normal_params, Params::SphereDetectionParameters *sphere_params);

	std::string getWindowName() const {
		return "Display: " + this->getCameraName();
	}

	std::string getCameraName() const {
		return this->getName() + " Camera " + std::to_string(this->camera_id);
	}

	int getCameraId() const {
		return camera_id;
	}

	cv::Point selectedFloorPoint{-1, -1};

	bool detect_circles{ false };
	bool show_color_stream{ false };
	bool is_enabled{ true };
	WalkingAverageMatrix walkingFrames{};
	WalkingAverageMatrix walkingEdges{}; 
	cv::Vec3f floorNormal;
private:
	int camera_id;
};
