#pragma once
#include <vector>
#include <chrono>
#include <string>
#include <memory>

#include <opencv2/opencv.hpp>
#include <GLCore/Camera.h>
#include <GLCore/Renderer.h>

#include "DepthCamera.h"
#include "obj/Logger.h"
#include "obj/SkeletonDetectorOpenPose.h"
#include "obj/SkeletonDetectorNuitrack.h"
#include "obj/SessionParameters.h"

class CameraHandler
{
public:
	CameraHandler(Camera *cam, Renderer *r, Logger::Logger *logger);
	~CameraHandler();

	void initAllCameras();
	void OnUpdate();
	void OnImGuiRender();
private:
	void showGeneralGui();
	void showRecordingGui();
	void showPlaybackGui();

	void showRecordingStats();
	void showRecordings();

	void initRecording();
	void countdown();
	void startRecording();

	void record();
	void stream();
	void playback();

	void fixSkeleton();

	void stopRecording();
	void startPlayback(Json::Value recording);
	void stopPlayback();
	void findRecordings();

	void calculateSkeletonsNuitrack(Json::Value recording);
	void calculateSkeletonsOpenpose(Json::Value recording);

	void clearCameras();
	void updateSessionName();

	enum State {
		Streaming,
		RecordingPre,
		Countdown,
		Recording,
		Playback
	};

	State m_State{ State::Streaming };

	Camera *mp_Camera;
	Renderer *mp_Renderer;

	Logger::Logger* mp_Logger;

	// Streaming
	bool m_CamerasExist{ false };
	bool m_ShowColorFrames{ false };
	std::unique_ptr<GLObject::PointCloud> mp_PointCloud;
	std::vector<DepthCamera *> m_DepthCameras;

	// Recording
	std::string m_SessionName{ };
	SessionParameters m_SessionParams{ };
	std::chrono::time_point<std::chrono::system_clock> m_RecordingStart;
	std::chrono::time_point<std::chrono::system_clock> m_RecordingEnd;
	std::chrono::duration<double> m_RecordedSeconds;
	int m_RecordedFrames{ 0 };

	// Playback
	std::vector<Json::Value> m_Recordings;
	bool m_FoundRecordedSkeleton{ false };
	Json::Value m_Recording;
	Json::Value m_RecordedSkeleton;
	cv::Mat m_CurrentColorFrame{ };
	bool m_FixSkeleton{ false };
	bool m_PlaybackPaused{ false };
	int m_TotalPlaybackFrames{ 0 };
	int m_CurrentPlaybackFrame{ 0 };

	// Skeleton Detection
	bool m_DoSkeletonDetection{ false };
	float m_ScoreThreshold{ 0.0f };
	bool m_ShowUncertainty{ false };
	bool m_UseNuitrack{ false };
	std::unique_ptr<SkeletonDetectorOpenPose> m_SkeletonDetectorOpenPose;
	std::unique_ptr<SkeletonDetectorNuitrack> m_SkeletonDetectorNuitrack;
};

