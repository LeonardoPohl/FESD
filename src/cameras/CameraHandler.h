#pragma once
#include <vector>
#include <chrono>
#include <json/json.h>

#include "DepthCamera.h"
#include "GLCore/Camera.h"
#include "GLCore/Renderer.h"
#include "obj/Logger.h"

class CameraHandler
{
public:
	CameraHandler(Camera *cam, Renderer *r, Logger::Logger *logger);
	~CameraHandler();

	void initAllCameras();
	void OnRender();
	void OnImGuiRender();
private:
	void showSessionSettings();
	void showRecordingStats();
	void showRecordings();
	void startRecording();
	void stopRecording();
	void findRecordings();

	void clearCameras() {
		for (auto cam : m_DepthCameras)
			delete cam;
		m_DepthCameras.clear();
	}

	void updateSessionName() {
		auto tp = std::chrono::system_clock::now();
		static auto const CET = std::chrono::locate_zone("Etc/GMT-1");
		m_SessionName = "Session " + std::format("{:%FT%T}", std::chrono::zoned_time{ CET, floor<std::chrono::seconds>(tp) });
	}

	std::string getFileSafeSessionName() {
		std::string sessionFileName = m_SessionName;
		std::ranges::replace(sessionFileName, ' ', '_');
		std::ranges::replace(sessionFileName, ':', '.');

		return sessionFileName;
	}

	enum State {
		Streaming,
		Recording,
		Playback
	};

	State m_State;
	std::string m_SessionName{ "Session" };

	Camera *mp_Camera;
	Renderer *mp_Renderer;

	Logger::Logger* mp_Logger;

	std::vector<DepthCamera *> m_DepthCameras;
	std::vector<Json::Value> m_Recordings;

	std::chrono::time_point<std::chrono::system_clock> m_RecordingStart;
	std::chrono::time_point<std::chrono::system_clock> m_RecordingEnd;
	std::chrono::duration<double> m_RecordedSeconds;

	bool m_StreamWhileRecording{ true };
	bool m_LimitFrames{ false };
	bool m_LimitTime{ true };

	int m_FrameLimit{ 100 };

	/// <summary>
	/// Time Limit in Seconds
	/// </summary>
	int m_TimeLimit{ 100 };

	int m_RecordedFrames{ 0 };
};

