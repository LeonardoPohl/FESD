#pragma once
#include <chrono>
#include <vector>
#include <queue>

#include <imgui.h>
#include <json/json.h>

#include "utilities/helper/ImGuiHelper.h"
#include "Exercise.h"

class SessionParameters {
public:
	SessionParameters() {
		exercises = Exercise::getPredefinedExercises();
		for (int i = 0; i < exercises.size(); i++) {
			selectExercises.push_back(new bool(false));
		}
	}

	bool manipulateSessionParameters() {
		ImGui::Begin("Session Parameters");
		if (ImGui::Button("All exercises")) {
			selectedAnyExercise = true;
			for (int i = 0; i < exercises.size(); i++) {
				*selectExercises[i] = true;
			}
		}

		for (int i = 0; i < exercises.size(); i++) {
			if (ImGui::Checkbox(exercises[i].Id.c_str(), selectExercises[i])) {
				bool selected = false;
				for (int i = 0; i < exercises.size(); i++) {
					selected |= *selectExercises[i];
				}
				selectedAnyExercise = selected;
			}
			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				exercises[i].imguiExercise();
				ImGui::EndTooltip();
			}
		}

		ImGui::BeginDisabled(selectedAnyExercise);
		ImGui::Separator();
		manualExercise.imguiExercise(true);
		ImGui::Separator();
		ImGui::EndDisabled();

		ImGui::Checkbox("Background close", &BackgroundClose);
		ImGui::Checkbox("Cramped", &Cramped);
		ImGui::Checkbox("Dark Clothing", &DarkClothing);
		ImGui::Separator();

		ImGui::Checkbox("Estimate Skeleton", &EstimateSkeleton);
		ImGuiHelper::HelpMarker("Estimate the skeleton while recording.");

		ImGui::Checkbox("Stream", &StreamWhileRecording);
		ImGuiHelper::HelpMarker("Show the Live Pointcloud while recording, this might decrease performance.");
		
		ImGui::Checkbox("Limit Frames", &LimitFrames);

		ImGui::BeginDisabled(!LimitFrames);
		ImGui::InputInt("Frame Limit", &FrameLimit, 1, 1000);
		if (FrameLimit < 0) {
			FrameLimit = 0;
			LimitFrames = false;
		}
		ImGui::EndDisabled();

		ImGui::Checkbox("Limit Time", &LimitTime);

		ImGui::BeginDisabled(!LimitTime);
		ImGui::InputInt("Time Limit (s)", &TimeLimitInS, 1, 100);
		if (TimeLimitInS < 0 || TimeLimitInS > 180) {
			TimeLimitInS = 0;
			LimitTime = false;
		}
		ImGui::EndDisabled();

		ImGui::Separator();
		ImGui::SliderInt("Countdown in S", &CountdownInS, 0, 10);
		ImGui::SliderInt("Repetitions", &RepeatNTimes, 1, 10);

		if (ImGui::Button("Begin Recording")) {
			Repetitions = 0;
			TotalExercises = 0;

			if (selectedAnyExercise) {
				for (int i = 0; i < exercises.size(); i++) {
					if (*selectExercises[i]) {
						TotalExercises += 1;
						selectedExercises.push(exercises[i]);
					}
				}
			}
			else {
				TotalExercises += 1;
				selectedExercises.push(manualExercise);
			}
			return true;
		}
		ImGui::End();

		return false;
	}

	explicit operator Json::Value () const {
		Json::Value val;

		val["Background close"] = BackgroundClose;
		val["Cramped"] = Cramped;
		val["Dark Clothing"] = DarkClothing;
		val["Exercise"] = selectedExercises.front().Id;

		return val;
	}

	/// <summary>
	/// Display a countdown before starting to record
	/// </summary>
	/// <returns>
	/// True - If the countdown is over and False - If the coundown is running
	/// </returns>
	bool countDown() {
		if (!CountdownStarted) {
			CountdownStarted = true;
			CountdownStart = std::chrono::system_clock::now();
		}

		if (CountdownInS == 0 ||
			CountdownInS <= std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - CountdownStart).count()) {
			CountdownStarted = false;
			return true;
		}

		ImGui::Begin("Countdown");
		selectedExercises.front().imguiExercise();
		char buf1[32];
		char buf2[32];

		ImGui::Text("Exercises:");
		sprintf(buf1, "Exercise %d/%d", (int)selectedExercises.size(), TotalExercises);
		ImGui::ProgressBar(1.0 - (double)selectedExercises.size() / (double)TotalExercises);
		ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);

		ImGui::Text("Repetitions:");
		sprintf(buf2, "Repetition %d/%d", Repetitions, RepeatNTimes);
		ImGui::ProgressBar((double)Repetitions / (double)RepeatNTimes);

		ImGui::ProgressBar(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - CountdownStart).count() / (double)CountdownInS);

		ImGui::End();

		return false;
	}

	/// <returns>true if all repetitions are done</returns>
	bool stopRecording() {
		Repetitions += 1;

		if (RepeatNTimes == Repetitions) {
			selectedExercises.pop();
			if (!selectedExercises.empty()) {
				Repetitions = 0;
			}
		}

		return RepeatNTimes == Repetitions;
	}


	bool StreamWhileRecording{ false };
	bool LimitFrames{ false };
	bool LimitTime{ true };
	bool EstimateSkeleton{ true };
	int RepeatNTimes{ 2 };
	int Repetitions{ 0 };
	int TotalExercises{ 0 };
	int FrameLimit{ 100 };
	int TimeLimitInS{ 20 };
private:
	bool BackgroundClose{ true };
	bool Cramped{ false };
	bool DarkClothing{ false };

	bool selectedAnyExercise{ false };
	Exercise manualExercise{ };
	std::vector<Exercise> exercises{ };
	std::vector<bool*> selectExercises{ };
	std::queue<Exercise> selectedExercises{ };

	bool CountdownStarted{ false };
	int CountdownInS{ 5 };
	std::chrono::time_point<std::chrono::system_clock> CountdownStart;
};