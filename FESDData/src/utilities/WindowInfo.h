#pragma once

#include <imgui.h>

class WindowInformation {
public:
	WindowInformation() {

	}

	inline void UpdateFps(float deltaTime) {
		m_Fps = (float)((m_Fps * m_FpsSmoothing) + (deltaTime * (1.0 - m_FpsSmoothing)));
	}

	void ShowInformation() {
        ImGui::Begin("Information");

        if (m_RefreshTime == 0.0)
            m_RefreshTime = ImGui::GetTime();

        static float phase = 0.0f;
        while (m_RefreshTime < ImGui::GetTime())
        {
            m_ContinuousFps[m_ValuesOffset] = 1.0f / m_Fps;
            m_ValuesOffset = (m_ValuesOffset + 1) % IM_ARRAYSIZE(m_ContinuousFps);
            phase += 0.10f * m_ValuesOffset;
            m_RefreshTime += 1.0f / 60.0f;
        }

        {
            float average = 0.0f;
            for (int n = 0; n < IM_ARRAYSIZE(m_ContinuousFps); n++)
                average += m_ContinuousFps[n];
            average /= (float)IM_ARRAYSIZE(m_ContinuousFps);
            char overlay[32];
            ImGui::Text("avg %.2f, curr %.2f", average, 1.0f / m_Fps);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PlotLines("", m_ContinuousFps, IM_ARRAYSIZE(m_ContinuousFps), m_ValuesOffset, overlay, 0.0f, 60.0f, ImVec2(0, 80.0f));
                ImGui::EndTooltip();
            }
        }

        ImGui::End();
	}

private:
	float m_Fps{ 0.0f };
	float m_FpsSmoothing{ 0.1f };

	float m_ContinuousFps[90]{ };
	int m_ValuesOffset{ 0 };
	double m_RefreshTime{ 0.0 };
};