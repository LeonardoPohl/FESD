#pragma once
#include <OpenNI.h>				// Include OpenNI
#include <opencv2/core.hpp>		// Include OpenCV
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <stdexcept>
#include <Circle.h>

struct SphereDetectionParameters {
	float sphere_radius = 50;

	float param1 = 10;
	float param2 = 10;
    int min_radius = 0;
    int max_radius = 10;
};

class DepthCamera {
public:
	virtual ~DepthCamera() = default;
	virtual cv::Mat getDepthFrame() = 0;
	virtual cv::Mat getColorFrame() = 0;
	virtual std::string getName() const = 0; 
	virtual bool hasColorStream() = 0;
	virtual cv::Point3f pixelToPoint(int x, int y, ushort depth) const = 0;

	std::vector<Circle*> detectSpheres(SphereDetectionParameters params);
	std::vector<Circle*> detectSpheres(cv::Mat frame, SphereDetectionParameters params);

	std::string getWindowName() const {
		return "Display: " + this->getCameraName();
	}

	std::string getCameraName() const {
		return this->getName() + " Camera " + std::to_string(this->camera_id);
	}

	int getCameraId() const {
		return camera_id;
	}

	bool detect_circles{ true };
	bool show_color_stream{ true };
	bool is_enabled{ true };

private:
	int camera_id;
};

class OrbbecCamera : public DepthCamera {
public:
	OrbbecCamera(const openni::DeviceInfo* deviceInfo, int camera_id);
	~OrbbecCamera() override;

	cv::Mat getDepthFrame() override;
	cv::Mat getColorFrame() override;
	bool hasColorStream() { return _device.hasSensor(openni::SENSOR_COLOR); };
	std::string getName() const override { return "Orbbec"; }
	cv::Point3f pixelToPoint(int x, int y, ushort depth) const override;

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

class RealSenseCamera : public DepthCamera {
public:
	RealSenseCamera(rs2::context* ctx, rs2::device* device, int camera_id);
	~RealSenseCamera() override;

	cv::Mat getDepthFrame() override;
	cv::Mat getColorFrame() override;
	bool hasColorStream() { return true; };
	std::string getName() const override { return "Realsense"; }
	cv::Point3f pixelToPoint(int x, int y, ushort depth) const override;

	void printDeviceInfo() const;

	static rs2::device_list getAvailableDevices(rs2::context ctx);
	static std::vector<RealSenseCamera*> initialiseAllDevices();
private:
	rs2::pipeline _pipe;
	rs2::context* _ctx{};
	rs2::device* _device{};
	rs2::config _cfg{};

	// Declare depth colorizer for pretty visualization of depth data
	rs2::colorizer _color_map{};
	int camera_id;
};
