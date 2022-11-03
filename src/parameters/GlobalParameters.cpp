#include "Parameters.h"

void Params::GlobalParameters::displayParameters() {
	ImGui::Begin("Global Settings");
	ImGui::Text("%d Devices Available", depth_cameras->size());

	ImGui::Text("");

	for (DepthCamera* cam : *depth_cameras) {
		ImGui::SameLine();
		ImGui::Checkbox(cam->getCameraName().c_str(), &cam->is_enabled);
	}

	ImGui::Checkbox("Display edges", &display_edges);
	ImGui::Checkbox("Calculate Surface Normals", &calculate_surface_normals);
	bool reset_walking_frames;
	reset_walking_frames |= ImGui::Checkbox("Use Walking Average", &walking_average);
	/*
	if (reset_walking_frames) {
		for (DepthCamera* cam : *depth_cameras) {
			cam->walkingFrames.reset();
			cam->walkingEdges.reset();
		}
	}*/

	ImGui::End();
}