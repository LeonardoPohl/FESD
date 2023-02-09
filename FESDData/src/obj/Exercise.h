#pragma once
#include <array>
#include <json/json.h>

class Exercise
{
public:
	enum class Difficulty;
	Exercise(){}
	Exercise(std::string id, std::string Description, Difficulty difficulty, bool sitting, bool holding_weight, bool ankle_weight) : Id(id)  {}
	
	static std::vector<Exercise> getPredefinedExercises();

	void imguiExercise() {
		ImGui::Separator();
		ImGui::Text("Exercise");
		ImGui::InputText("Id", &Id);
		ImGui::InputTextMultiline("Description", &Description);

		const char* state_name = DifficultyNames[DifficultyElem];
		if (ImGui::SliderInt("State", &DifficultyElem, 0, DifficultyCount - 1, state_name))
			difficulty = static_cast<Difficulty>(DifficultyElem);

		ImGui::Checkbox("Sitting", &Sitting);
		ImGui::Checkbox("Holding Weight", &Holding_Weight);
		ImGui::Checkbox("Ankle Weight", &Ankle_Weight);
		ImGui::Separator();
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

// TODO: write a file that does this
std::vector<Exercise> Exercise::getPredefinedExercises() {
	std::vector<Exercise> exercises;
	//std::string id, std::string Description, bool sitting, bool holding_weight, bool ankle_weight

	for (const auto& entry : std::filesystem::directory_iterator(m_RecordingDirectory))
	{
		if (entry.path().filename().string().find("Exercise.json") == std::string::npos) {
			std::ifstream configJson(entry.path());
			Json::Value root;

			Json::CharReaderBuilder builder;

			builder["collectComments"] = true;
			JSONCPP_STRING errs;
			for (auto exercise : root["Exercises"]) {
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