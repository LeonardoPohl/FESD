#pragma once
#include <array>
#include <filesystem>
#include <fstream>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <json/json.h>

#include "utilities/Consts.h"

class Exercise
{
public:
	enum class Difficulty;
	Exercise(){}
	Exercise(std::string id, std::string description, Difficulty difficulty, bool sitting, bool holding_weight, bool ankle_weight) 
		: Id(id), Description(description), difficulty(difficulty), Sitting(sitting), Holding_Weight(holding_weight), Ankle_Weight(ankle_weight)  {}
	
	static std::vector<Exercise> getPredefinedExercises() {
		std::vector<Exercise> exercises;
		//std::string id, std::string Description, bool sitting, bool holding_weight, bool ankle_weight

		for (const auto& entry : std::filesystem::directory_iterator(m_RecordingDirectory))
		{
			if (entry.path().filename().string().find("Exercises.json") != std::string::npos) {
				std::ifstream configJson(entry.path());
				Json::Value root;
				configJson >> root;

				auto es = root["Exercises"];				
				for (auto exercise : es) {
					Difficulty difficulty;
					switch (exercise["Id"].asString()[2])
					{
					case '0':
						difficulty = Difficulty::Trivial;
						break;
					case '1':
						difficulty = Difficulty::Easy;
						break;
					case '2':
						difficulty = Difficulty::Medium;
						break;
					case '3':
						difficulty = Difficulty::Hard;
						break;
					default:
						difficulty = Difficulty::Trivial;
						break;
					}
					exercises.push_back({
							exercise["Id"].asString(),
							exercise["Description"].asString(),
							difficulty,
							exercise["Sitting"].asBool(),
							exercise["Ankle Weight"].asBool(),
							exercise["Holding Weight"].asBool()
						});
				}
			}
		}

		return exercises;
	}

	void imguiExercise(bool editable = false) {
		ImGui::Text("Exercise");
		if (editable) {
			ImGui::InputText("Id", &Id);
			ImGui::InputTextMultiline("Description", &Description);
		}
		else {
			ImGui::Text(Id.c_str());
			ImGui::TextWrapped(Description.c_str());
		}

		const char* state_name = DifficultyNames[DifficultyElem];
		if (ImGui::SliderInt("State", &DifficultyElem, 0, DifficultyCount - 1, state_name))
			difficulty = static_cast<Difficulty>(DifficultyElem);

		ImGui::Checkbox("Sitting", &Sitting);
		ImGui::Checkbox("Holding Weight", &Holding_Weight);
		ImGui::Checkbox("Ankle Weight", &Ankle_Weight);
	}

	explicit operator Json::Value() const {
		Json::Value val;

		val["Id"] = Id;
		
		return val;
	}

	bool PreDefined{ false };

	enum class Difficulty {
		Trivial,
		Easy,
		Medium,
		Hard
	};

	static const int DifficultyCount = 4;

	const std::array<const char*, DifficultyCount> DifficultyNames{ "Trivial", "Easy", "Medium", "Hard"};

	Difficulty difficulty{ Difficulty::Easy };
	int DifficultyElem{ 1 };

	bool Sitting{ false };
	bool Holding_Weight{ false };
	bool Ankle_Weight{ false };

	std::string Id;
	std::string Description;
};
 