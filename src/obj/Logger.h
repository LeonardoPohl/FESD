#pragma once
#include <string>
#include <chrono>
#include <imgui.h>
#include <iostream>

namespace Logger {
	enum class LogLevel
	{
		INFO,
		WARNING,
		ERR
	};

	class Logger {
	public:
		#pragma warning(disable : 4996)
		void log(std::string msg, LogLevel level = LogLevel::INFO) {
			std::string entry = "";
			auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			entry += ctime(&now);
			entry.at(entry.size() - 1) = ' ';
			entry += "- ";

			if (level == LogLevel::INFO) {
				entry += "[INFO] ";
			}
			else if (level == LogLevel::WARNING) {
				entry += "[WARN] ";
			}
			else if (level == LogLevel::ERR) {
				entry += "[ERROR] ";
			}

			entry += msg;
			entry += "\n";

			std::cout << entry << std::endl;

			m_Log = entry + m_Log;

			if (m_Log.size() > m_MaxLogLength) {
				m_Log.resize(9000);
			}
		}

		void showLog() {
			ImGui::Begin("Log");
			ImGui::Text(m_Log.c_str());
			ImGui::End();
		}
	private:
		std::string m_Log{ "" };
		const int m_MaxLogLength{ 10000 };
	};
}