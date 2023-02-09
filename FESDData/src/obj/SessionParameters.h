#pragma once
#include <chrono>

#include <imgui.h>
#include <json/json.h>

#include "utilities/helper/ImGuiHelper.h"
#include "Exercise.h"

class SessionParameters {
public:
	SessionParameters() {
		
	}
	bool manipulateSessionParameters() {
		ImGui::Begin("Session Parameters");

		exercise.imguiExercise();

		ImGui::Checkbox("Background close", &Background_Close);
		ImGui::Checkbox("Cramped", &Cramped);
		ImGui::Checkbox("Dark Clothing", &Dark_Clothing);
		ImGui::Separator();

		ImGui::Checkbox("Stream While Recording", &StreamWhileRecording);
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
		if (TimeLimitInS < 0) {
			TimeLimitInS = 0;
			LimitTime = false;
		}
		ImGui::EndDisabled();

		ImGui::Separator();
		ImGui::SliderInt("Countdown in S", &CountdownInS, 0, 10);
		ImGui::SliderInt("Repetitions", &RepeatNTimes, 0, 10);

		if (ImGui::Button("Begin Recording")) {
			Repetitions = 0;
			return true;
		}
		ImGui::End();

		return false;
	}

	explicit operator Json::Value () const {
		Json::Value val;

		val["Background close"] = Background_Close;
		val["Cramped"] = Cramped;
		val["Dark Clothing"] = Dark_Clothing;
		val["Height"] = Height;
		val["Angle"] = Angle;
		val["Exercise"] = exercise.Id;

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
		ImGui::ProgressBar(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - CountdownStart).count() / (double)CountdownInS);
		ImGui::End();

		return false;
	}

	/// <summary>
	/// 
	/// </summary>
	/// <returns>true if all repetitions are done</returns>
	bool stopRecording() {
		return RepeatNTimes == ++Repetitions;
	}


	bool StreamWhileRecording{ false };
	bool LimitFrames{ false };
	bool LimitTime{ true };
	int RepeatNTimes{ 1 };
	int Repetitions{ 0 };
	int FrameLimit{ 100 };
	int TimeLimitInS{ 20 };
private:
	bool Background_Close{ true };
	bool Cramped{ false };
	bool Dark_Clothing{ true };	

	std::vector<Exercise> exercise{ };
	std::vector<Exercise> exercises{ };

	float Height{ 1.75f };
	float Angle{ 20.0 };

	bool CountdownStarted{ false };
	int CountdownInS{ 5 };
	std::chrono::time_point<std::chrono::system_clock> CountdownStart;
};