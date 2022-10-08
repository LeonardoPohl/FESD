#include <Parameters.h>

void Params::SphereDetectionParameters::displayParameters() {
	ImGui::Begin("Sphere Detection Parameters");

	ImGui::Text("Edge Detection Settings");

	const char* adaptive_threshold_types[] = {
		"Mean",
		"Gaussian"
	};
	const char* threshold_types[] = {
		"Binary",
		"Binary Inverted",
		"Trunc",
		"To Zero",
		"To Zero Inverted",
		"Mask",
		"Otsu",
		"Triangle"
	};

	if (ImGui::Combo("Adaptive Threshold Type", &current_adaptive_threshold, adaptive_threshold_types, IM_ARRAYSIZE(adaptive_threshold_types))) {
		adapriveThresholdType = adapriveThresholdTypes[current_adaptive_threshold];
	}

	if (ImGui::Combo("Threshold Type", &current_threshold, threshold_types, IM_ARRAYSIZE(threshold_types))) {
		thresholdType = thresholdTypes[current_threshold];
	}

	ImGui::Separator();
	ImGui::Text("Sphere Detector Settings");

	ImGui::SliderFloat("Sphere Radius", &sphere_radius, 0.1f, 100);
	ImGui::DragIntRange2("Circle Radius", &min_radius, &max_radius, 5, 0, 100, "Min: %d", "Max: %d");
	ImGui::SliderFloat("Canny edge detector threshold", &param1, 1, 500);
	ImGui::SliderFloat("Accumulator threshold", &param2, 1, 500);

	ImGui::Separator();
	ImGui::End();
}