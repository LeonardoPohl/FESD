#pragma once
#include "DepthCamera.h"
#include "json/json.h"
class NuiPlaybackCamera : public DepthCamera
{
public:
	/// Constructors & Destructors
	NuiPlaybackCamera(Camera* cam, Renderer* renderer, Logger::Logger* logger, std::filesystem::path recording, int* currentPlaybackFrame, Json::Value camera);
	~NuiPlaybackCamera() override;

	/// Camera Details
	static std::string getType();
	inline std::string getCameraName() const override;
	void showCameraInfo() override;
	void printDeviceInfo() const;
	unsigned int getDepthStreamWidth() const override { return m_DepthWidth; }
	unsigned int getDepthStreamHeight() const override { return m_DepthHeight; }
	float getIntrinsics(INTRINSICS intrin) const override;
	glm::mat3 getIntrinsics() const override;
	float getMetersPerUnit() const override;

	/// Frame retreival
	void queryFrame();
	const void* getDepth() override;
	cv::Mat getColorFrame() override;

	/// Camera Settings
	void CameraSettings() override {};

	/// Recording
	std::string startRecording(std::string sessionName) override { mp_Logger->log("NuiPlayback does not support Recording", Logger::Priority::ERR); return ""; };
	void saveFrame() override { mp_Logger->log("NuiPlayback does not support Recording", Logger::Priority::ERR); };
	void stopRecording() override { mp_Logger->log("NuiPlayback does not support Recording", Logger::Priority::ERR); };
private:
	Logger::Logger* mp_Logger;
	int m_QueriedFrame{ -1 };
	int* mp_CurrentPlaybackFrame;

	cv::FileStorage m_Frames{};

	cv::Mat m_CurrentDepthFrame{};
	cv::Mat m_CurrentColorFrame{};

	std::filesystem::path m_RecordingPath{ };

	unsigned int m_DepthWidth{ 0 };
	unsigned int m_DepthHeight{ 0 };

	float m_fx{ 0.f };
	float m_fy{ 0.f };
	float m_cx{ 0.f };
	float m_cy{ 0.f };

	float m_MetersPerUnit{ 0.00025f };
};

