#pragma once
#include <vector>
#include "DepthCamera.h"
#include "GLCore/Camera.h"
#include "GLCore/Renderer.h"

#include <chrono>
#include <json/json.h>

class CameraHandler
{
public:
	CameraHandler(Camera *cam, Renderer *r);
	~CameraHandler();

	void findAllCameras();
	void initAllCameras();
	void showCameras();
	void OnRender();
	void OnImGuiRender();

	void UpdateSessionName() {
		auto tp = std::chrono::system_clock::now();
		static auto const CET = std::chrono::locate_zone("Etc/GMT-1");
		m_SessionName = "Session " + std::format("{:%FT%T}", std::chrono::zoned_time{CET, floor<std::chrono::seconds>(tp)});
	}

	std::string getFileSafeSessionName(){
		std::string sessionFileName = m_SessionName;
		std::ranges::replace(sessionFileName, ' ', '_');
		std::ranges::replace(sessionFileName, ':', '.');

		return sessionFileName;
	}

private:
	enum State {
		Streaming,
		Recording
	};

	State m_State;
	std::string m_SessionName{ "Session" };

	Camera *mp_Camera;
	Renderer *mp_Renderer;

	std::vector<DepthCamera *> m_DepthCameras;
	std::vector<Json::Value> m_Recordings;
	
	bool m_DoingPlayback{ false };
	bool m_Recording{ false };

	// For playback to seek the position
	int m_SeekPosition{ 0 };
};

