#pragma once
#include <array>
#include <json/json.h>

class Exercise
{
public:
	void imguiExercise() {
		ImGui::Separator();
		ImGui::Text("Exercise");
		ImGui::InputText("Name", Name, 100);

		const char* state_name = m_DifficultyNames[m_DifficultyElem];
		if (ImGui::SliderInt("State", &m_DifficultyElem, 0, m_DifficultyCount - 1, state_name))
			difficulty = static_cast<Difficulty>(m_DifficultyElem);

		ImGui::Checkbox("Sitting", &Sitting);
		ImGui::Checkbox("Holding Weight", &Holding_Weight);
		ImGui::Checkbox("Ankle Weight", &Ankle_Weight);
		ImGui::Separator();
	}

	explicit operator Json::Value() const {
		Json::Value val;

		val["Name"] = Name;
		val["Difficulty"] = m_DifficultyNames[m_DifficultyElem];
		val["Sitting"] = Sitting;
		val["Holding Weight"] = Holding_Weight;
		val["Ankle Weight"] = Ankle_Weight;
		
		return val;
	}

	enum class Difficulty {
		Trivial,
		Easy,
		Medium,
		Hard
	};

	static const int m_DifficultyCount = 4;

	const std::array<const char*, m_DifficultyCount> m_DifficultyNames{ "Trivial", "Easy", "Medium", "Hard"};

	Difficulty difficulty{ Difficulty::Easy };
	int m_DifficultyElem{ 1 };

	bool Sitting{ false };
	bool Holding_Weight{ false };
	bool Ankle_Weight{ false };

	char Name[100];
};

