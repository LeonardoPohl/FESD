#pragma once
#include <stdexcept>
#include <string>

namespace Params {
	class GlobalParameters;
	class SphereDetectionParameters;
	class NormalParameters;
}

class DepthCamera {
public:
	virtual ~DepthCamera() = default;
	virtual const uint16_t *getDepth() = 0;
	virtual std::string getName() const = 0; 

	virtual unsigned int getDepthStreamWidth() const = 0;
	virtual unsigned int getDepthStreamHeight() const = 0;
	virtual unsigned int getDepthStreamMaxDepth() const = 0;

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

};
