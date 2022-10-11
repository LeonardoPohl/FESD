#pragma once
#include "DepthCamera.h"
#include <OpenNI.h>

class OrbbecCamera : public DepthCamera {
public:
	OrbbecCamera(const openni::DeviceInfo* deviceInfo, int camera_id);
	~OrbbecCamera() override;

	const void* getDepth() override;
	std::string getName() const override { return "Orbbec"; }

	void printDeviceInfo() const;

	static void getAvailableDevices(openni::Array<openni::DeviceInfo>* available_devices);
	static std::vector<OrbbecCamera*> initialiseAllDevices(int *starting_id);
private:
	const openni::DeviceInfo* _device_info;
	openni::Device _device;
	openni::VideoStream _depth_stream;
	openni::VideoStream _color_stream;
	openni::VideoFrameRef _frame_ref;
	openni::Status rc;
};
