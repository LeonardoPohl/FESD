#include <Parameters.h>


void Params::NormalParameters::displayParameters() {
	ImGui::Begin("Normal Settings");

	ImGui::SliderInt("Whats Up", &whatsUp, 0, 2);
	ImGui::SliderFloat("How Up", &upnessFilter, 0, 2 * 3.45f);
	ImGui::SliderInt("Number of Samples", &num_samples, 0, 50);
	ImGui::SliderFloat("Edge cutoff", &edgeCutoff, 1, 50);

	ImGui::End();
}