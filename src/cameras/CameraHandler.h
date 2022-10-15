#pragma once
#include <vector>
#include "DepthCamera.h"

namespace Params
{
	class GlobalParameters;
	class SphereDetectionParameters;
	class NormalParameters;
}
class CameraHandler
{
public:
	CameraHandler();
	~CameraHandler();

	void findAllCameras();
	void initAllCameras();
	void showCameras();
	void OnImGuiRender();
private:
	std::vector<DepthCamera *> depthCameras;

	std::unique_ptr<Params::GlobalParameters> global_params;
	std::unique_ptr<Params::NormalParameters> normal_params;
};

