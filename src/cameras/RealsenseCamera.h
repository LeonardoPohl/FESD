#pragma once
#include <memory>

#include <librealsense2/rs.hpp>

#include "DepthCamera.h"

class RealSenseCamera : public DepthCamera {
public:
	/// Constructors & Destructors
	RealSenseCamera(rs2::context* ctx, rs2::device* device, Camera *cam, Renderer *renderer, int camera_id, Logger::Logger* logger);
	RealSenseCamera(Camera* cam, Renderer* renderer, Logger::Logger* logger, std::filesystem::path recording);
	~RealSenseCamera() override;

	/// Initialise all devices
	static rs2::device_list getAvailableDevices(rs2::context ctx);
	static std::vector<RealSenseCamera*> initialiseAllDevices(Camera* cam, Renderer* renderer, int* starting_id, Logger::Logger* logger);

	/// Camera Details
	static std::string getType();
	inline std::string getCameraName() const override;
	void showCameraInfo() override;
	void printDeviceInfo() const;
	inline unsigned int getDepthStreamWidth() const override { return m_DepthWidth; }
	inline unsigned int getDepthStreamHeight() const override { return m_DepthHeight; }
	inline float getIntrinsics(INTRINSICS intrin) const override;
	inline glm::mat3 getIntrinsics() const override;
	inline float getMetersPerUnit() const override;

	/// Frame retreival
	const void *getDepth() override;
	cv::Mat getColorFrame() override;

	/// Camera Settings
	void CameraSettings() override {};

	/// Recording
	std::string startRecording(std::string sessionName) override;
	void saveFrame() override;
	void stopRecording() override;
private:
	std::shared_ptr<rs2::pipeline> mp_Pipe;
	rs2::context* mp_Context{};
	rs2::device m_Device{};
	rs2::config m_Config{};

	rs2::align m_AlignToDepth{ RS2_STREAM_DEPTH };

	rs2_intrinsics m_Intrinsics;

	Logger::Logger* mp_Logger;

	unsigned int m_DepthWidth;
	unsigned int m_DepthHeight;
	float m_MetersPerUnit;
};
