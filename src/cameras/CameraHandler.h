#pragma once
#include <vector>
#include "DepthCamera.h"
#include "GLCore/Camera.h"
#include "GLCore/Renderer.h"

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
private:
	enum State {
		Streaming,
		Recording
	};

	State m_State;
	char m_SessionName[120]{ "Session" };

	Camera *mp_Camera;
	Renderer *mp_Renderer;

	std::vector<DepthCamera *> m_DepthCameras;
	std::vector<Json::Value> m_Recordings;
	
	bool m_DoingPlayback{ false };

	// For playback to seek the position
	int m_SeekPosition{ 0 };
};

