#pragma once
#include <vector>
#include "DepthCamera.h"
#include "GLCore/Camera.h"
#include "GLCore/Renderer.h"

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
	Camera *cam;
	Renderer *renderer;

	std::vector<DepthCamera *> depthCameras;
};

