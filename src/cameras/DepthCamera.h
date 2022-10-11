#pragma once
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
	virtual const void *getDepth() = 0;
	virtual std::string getName() const = 0; 

	inline unsigned int getDepthStreamWidth() const
	{
		return depth_width;
	}	

	inline unsigned int getDepthStreamHeight() const
	{
		return depth_height;
	}

	inline unsigned int getDepthStreamDepth() const
	{
		return max_depth;
	}

	inline std::string getWindowName() const {
		return "Display: " + this->getCameraName();
	}

	inline std::string getCameraName() const {
		return this->getName() + " Camera " + std::to_string(this->camera_id);
	}

	inline unsigned int getCameraId() const
	{
		return camera_id;
	}

	bool is_enabled{ true };
protected:
	unsigned int camera_id;

	unsigned int max_depth;

	unsigned int depth_width;
	unsigned int depth_height;
};
