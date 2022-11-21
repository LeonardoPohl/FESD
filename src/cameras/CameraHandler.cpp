#include "CameraHandler.h"

#include <imgui.h>

#include "RealsenseCamera.h"
#include "OrbbecCamera.h"

#include <OpenNI.h>
#include <iostream>
#include <obj/PointCloud.h>


CameraHandler::CameraHandler(Camera *cam, Renderer *renderer) : cam(cam), renderer(renderer)
{
    if (openni::OpenNI::initialize() != openni::STATUS_OK)
        printf("Initialization of OpenNi failed\n%s\n", openni::OpenNI::getExtendedError());
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
    auto rs_cameras = RealSenseCamera::initialiseAllDevices(cam, renderer, &id);
    auto orbbec_cameras = OrbbecCamera::initialiseAllDevices(cam, renderer, &id);

    std::cout << "[INFO] Queried all devices" << std::endl;

    depthCameras.insert(depthCameras.end(), rs_cameras.begin(), rs_cameras.end());
    depthCameras.insert(depthCameras.end(), orbbec_cameras.begin(), orbbec_cameras.end());
}

void CameraHandler::showCameras()
{
    // TODO: Implement
    for (auto cam : depthCameras)
    {
        ImGui::Checkbox(cam->getCameraName().c_str(), &cam->is_enabled);        
    }
}

void CameraHandler::OnRender()
{
    for (auto cam : depthCameras)
    {
        if (cam->is_enabled)
        {
            cam->OnUpdate();
            cam->OnRender();
        }
    }
}

void CameraHandler::OnImGuiRender()
{
    //# General Camera Window
    //#######################
    ImGui::Begin("Camera Handler");

    if (ImGui::Button("Init Cameras"))
    {
        // TODO: Make Async (#21 Async Camera Initialisation)
        initAllCameras();
    }

    // TODO: Implement (#21 Find and not initialise all cameras)
    //cameraHandler.showCameras();

    for (auto cam : depthCameras)
    {
        ImGui::Begin(cam->getCameraName().c_str());
        ImGui::Checkbox(cam->getCameraName().c_str(), &cam->is_enabled);
        if (cam->is_enabled)
        {
            cam->OnImGuiRender();
        }
        ImGui::End();
    }

    ImGui::End();
}