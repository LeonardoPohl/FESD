#pragma once
#include <OpenNI.h>
#include <opencv2/core.hpp>

class DepthCamera {
public:
	DepthCamera();
	virtual ~DepthCamera();
	virtual cv::Mat getFrame();
	static std::vector<std::string> getAvailableDeviceUri();
};

class Orbbec : public DepthCamera {
public:
	Orbbec();
	Orbbec(std::string uri);
	~Orbbec();
	cv::Mat getFrame();
	static std::vector<std::string> getAvailableDeviceUri();
private:
	openni::DeviceInfo _deviceInfo;
};

class RealSense : public DepthCamera {
public:
	RealSense();
	RealSense(std::string uri);
	~RealSense();
	cv::Mat getFrame();
	static std::vector<std::string> getAvailableDeviceUri();
};