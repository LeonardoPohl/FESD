#pragma once
#include <OpenNI.h>				// Include OpenNI
#include <opencv2/core.hpp>		// Include OpenCV
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <stdexcept>

#include <utilities/Circle.h>
#include <utilities/WalkingAverage.h>

namespace Params {
	class GlobalParameters;
	class SphereDetectionParameters;
	class NormalParameters;
}


class DepthCamera {
public:
	virtual ~DepthCamera() = default;
	virtual void *getDepth();
	virtual std::string getName() const = 0; 

	int getDepthStreamWidth() const
	{
		return depth_width;
	}	

	int getDepthStreamHeigth() const
	{
		return depth_height;
	}

	int getDepthStreamDepth() const
	{
		return max_depth;
	}


	std::string getWindowName() const {
		return "Display: " + this->getCameraName();
	}

	std::string getCameraName() const {
		return this->getName() + " Camera " + std::to_string(this->camera_id);
	}

	int getCameraId() const
	{
		return camera_id;
	}

	bool is_enabled{ true };
private:
	int camera_id;

	int max_depth;

	int depth_width;
	int depth_height;
};
