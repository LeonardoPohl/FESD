#include "CameraHandler.h"

#include <imgui.h>

#include <parameters/Parameters.h>
#include "RealsenseCamera.h"
#include "OrbbecCamera.h"

#include <OpenNI.h>
#include <iostream>
#include <obj/PointCloud.h>


CameraHandler::CameraHandler(Camera *cam) : cam(cam)
{
    if (openni::OpenNI::initialize() != openni::STATUS_OK)
        printf("Initialization of OpenNi failed\n%s\n", openni::OpenNI::getExtendedError());

    global_params = std::make_unique<Params::GlobalParameters>(&depthCameras);
    normal_params = std::make_unique<Params::NormalParameters>();
}

CameraHandler::~CameraHandler()
{
    for (auto cam : depthCameras)
    {
        delete cam;
    }
    openni::OpenNI::shutdown();
}

void CameraHandler::findAllCameras()
{
    // TODO: Implement find all cameras
}

void CameraHandler::initAllCameras()
{
    depthCameras.clear();

    int id = 0;
    auto rs_cameras = RealSenseCamera::initialiseAllDevices(cam, &id);
    auto orbbec_cameras = OrbbecCamera::initialiseAllDevices(cam, &id);

    std::cout << "[INFO] Queried all devices" << std::endl;

    depthCameras.insert(depthCameras.end(), rs_cameras.begin(), rs_cameras.end());
    depthCameras.insert(depthCameras.end(), orbbec_cameras.begin(), orbbec_cameras.end());
}

void CameraHandler::showCameras()
{
    for (auto cam : depthCameras)
    {
        ImGui::Checkbox(cam->getCameraName().c_str(), &cam->is_enabled);        
    }
}

void CameraHandler::OnImGuiRender()
{
    for (auto cam : depthCameras)
    {
        ImGui::Checkbox(cam->getCameraName().c_str(), &cam->is_enabled);
        if (cam->is_enabled)
        {
            cam->OnUpdate();
            cam->OnRender();
            cam->OnImGuiRender();
        }
    }
}