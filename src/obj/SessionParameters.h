#pragma once
#include <chrono>

#include <imgui.h>
#include <json/json.h>

#include "utilities/helper/ImGuiHelper.h"

class SessionParameters {
public:
	bool manipulateSessionParameters() {
		ImGui::Begin("Session Parameters");
		ImGui::Checkbox("Sitting", &Sitting);
		ImGui::Checkbox("Background close", &Background_Close);
		ImGui::Checkbox("Cramped", &Cramped);
		ImGui::Checkbox("Dark Clothing", &Dark_Clothing);
		ImGui::Checkbox("Holding Weight", &Holding_Weight);
		ImGui::Checkbox("Ankle Weight", &Ankle_Weight);
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

		if (ImGui::Button("Begin Recording")) {
			CountdownStart = std::chrono::system_clock::now();
			return true;
		}
		ImGui::End();

		return false;
	}

	explicit operator Json::Value () const {
		Json::Value val;

		val["Sitting"] = Sitting;
		val["Background close"] = Background_Close;
		val["Cramped"] = Cramped;
		val["Dark Clothing"] = Dark_Clothing;
		val["Holding Weight"] = Holding_Weight;
		val["Ankle Weight"] = Ankle_Weight;
		val["Height"] = Height;
		val["Angle"] = Angle;

		return val;
	}

	bool countDown() {
		if (CountdownInS <= (std::chrono::system_clock::now() - CountdownStart).count())
			return true;
		ImGui::Begin("Countdown");
		ImGui::ProgressBar((double)CountdownInS / (std::chrono::system_clock::now() - CountdownStart).count());
		ImGui::End();

		return false;
	}

	bool StreamWhileRecording{ false };
	bool LimitFrames{ false };
	bool LimitTime{ true };
	int FrameLimit{ 100 };
	int TimeLimitInS{ 30 };
private:
	bool Sitting{ true };
	bool Background_Close{ true };
	bool Cramped{ false };
	bool Dark_Clothing{ true };
	bool Holding_Weight{ false };
	bool Ankle_Weight{ false };

	float Height{ 1.8f };
	float Angle{ 20.0 };

	int CountdownInS;
	std::chrono::time_point<std::chrono::system_clock> CountdownStart;
};