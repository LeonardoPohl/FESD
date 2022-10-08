#pragma once
#include <DepthCamera.h>

class OrbbecCamera : public DepthCamera {
public:
	OrbbecCamera(const openni::DeviceInfo* deviceInfo, int camera_id);
	~OrbbecCamera() override;

	cv::Mat getDepthFrame() override;
	cv::Mat getColorFrame() override;
	bool hasColorStream() { return _device.hasSensor(openni::SENSOR_COLOR); };
	std::string getName() const override { return "Orbbec"; }
	cv::Vec3f pixelToPoint(int x, int y, ushort depth) const override;

	void printDeviceInfo() const;

	static void getAvailableDevices(openni::Array<openni::DeviceInfo>* available_devices);
	static std::vector<OrbbecCamera*> initialiseAllDevices();
private:
	const openni::DeviceInfo* _device_info;
	openni::Device _device;
	openni::VideoStream _depth_stream;
	openni::VideoStream _color_stream;
	openni::VideoFrameRef _frame_ref;
	openni::Status rc;
	int camera_id;
};
