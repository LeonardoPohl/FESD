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

	void initAllCameras();
	void showCameras();
private:
	std::vector<DepthCamera *> depthCameras;

	std::unique_ptr<Params::GlobalParameters> global_params;
	std::unique_ptr<Params::SphereDetectionParameters> sphere_params;
	std::unique_ptr<Params::NormalParameters> normal_params;
};

