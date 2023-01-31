#pragma once
#include <imgui.h>
#include <json/json.h>

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
				
		if (ImGui::Button("Begin Recording")) {
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

private:
	bool Sitting{ true };
	bool Background_Close{ true };
	bool Cramped{ false };
	bool Dark_Clothing{ true };
	bool Holding_Weight{ false };
	bool Ankle_Weight{ false };

	float Height{ 1.8f };
	float Angle{ 20.0 };
};