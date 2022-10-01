#include <DepthCamera.h>
#include <iostream>
#include <opencv2/highgui.hpp>

void DepthCamera::doUpdate(
    Params::GlobalParameters* global_params, 
    Params::NormalParameters* normal_params, 
    Params::SphereDetectionParameters* sphere_params)
{
    cv::Mat depth_frame, edge_frame, color_frame, frame, normal_frame, floor_points;
    std::vector<int>::iterator new_end;
    try {
        depth_frame = this->getDepthFrame();

        if (global_params->walking_average) {
            this->walkingFrames.enqueue(depth_frame);
            depth_frame = this->walkingFrames.getValue();
        }

        if (this->show_color_stream) {
            color_frame = this->getColorFrame();
            if (color_frame.rows > 0 && color_frame.cols > 0) {
                cv::imshow(this->getWindowName() + " - Color Stream", color_frame);
                cv::resizeWindow(this->getWindowName() + " - Color Stream", color_frame.size[1], color_frame.size[0]);
            }
        }

        //# Display image
        //###############

        if (global_params->display_edges) {
            edge_frame = DepthCamera::detectEdges(depth_frame, sphere_params);
            if (global_params->walking_average) {
                this->walkingEdges.enqueue(edge_frame);
                edge_frame = this->walkingEdges.getValue();
            }

            if (edge_frame.rows > 0 && edge_frame.cols > 0) {
                cv::imshow(this->getWindowName() + " - Edges", edge_frame);
                cv::resizeWindow(this->getWindowName() + " - Edges", edge_frame.size[1], edge_frame.size[0]);
            }
        }

        //# Sphere Detection
        //##################

        if (this->detect_circles) {
            this->displaySphereTable(depth_frame, edge_frame, sphere_params, global_params->display_edges);
        }

        if (this->selectedFloorPoint.x != -1) {
            floor_points = this->calculateSelectedFloor(depth_frame, normal_params);

            if (floor_points.rows > 0 && floor_points.cols > 0) {
                cv::imshow(this->getWindowName() + " Floor", floor_points);
                cv::resizeWindow(this->getWindowName() + " Floor", floor_points.size[1], floor_points.size[0]);
            }
        }
        if (depth_frame.rows > 0 && depth_frame.cols > 0) {
            cv::imshow(this->getWindowName(), depth_frame);
            cv::resizeWindow(this->getWindowName(), depth_frame.size[1], depth_frame.size[0]);
        }
    }
    catch (cv::Exception e) {
        std::cout << " | " << e.msg;
        if (this->detect_circles) {
            this->detect_circles = false;
        }
        else {
            this->is_enabled = false;
        }
    }
    catch (std::exception e) {
        std::cout << " | " << e.what();
        if (this->detect_circles) {
            this->detect_circles = false;
        }
        else {
            this->is_enabled = false;
        }
    }
}