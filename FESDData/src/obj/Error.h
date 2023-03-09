#pragma once
#include <string>
#include <filesystem>
#include <format>

#include <json/json.h>

#include "utilities/Consts.h"
#include <iostream>
#include <imgui.h>

class Error {
public:
	Error(Json::Value error) {
		id = error["Id"].asInt();
		name = error["Name"].asString();
	}

	int id;
	std::string name;
};

static enum class ErrorType {
	Joint,
	Skleleton
};

struct Errors
{
public:
	Errors(ErrorType et) : errorType(et){
		std::filesystem::path file{"resources"};

		if (et == ErrorType::Joint) {
			file /= "JointErrors.json";
		}
		else if (et == ErrorType::Skleleton) {
			file /= "SkeletonErrors.json";
		}
		if (!std::filesystem::exists(file)) {
			std::cout << file.filename() << " does not exist. Errors could not be loaded!" << std::endl;
		}

		std::ifstream configJson(file);
		Json::Value root;

		Json::CharReaderBuilder builder;

		builder["collectComments"] = true;

		JSONCPP_STRING errs;

		if (!parseFromStream(builder, configJson, &root, &errs)) {
			std::cout << errs << std::endl;
			return;
		}
		for (auto err : root) {
			errors.push_back({ err });
		}
	}

	void Slider(int* err_id, int skel_id, int joint_id = -1) {
		auto slider_id = std::format("##{}.{}", skel_id, joint_id).c_str();

		if (ImGui::SliderInt(slider_id, err_id, 1, errors.size() - 1, errors[*err_id].name.c_str())) {
			std::cout << errors.size() << std::endl;
		}
	}

	Error operator[](int i) {
		return errors[i];
	}

	std::vector<Error> errors;
	ErrorType errorType;
};

static Errors JointErrors{ ErrorType::Joint };
static Errors SkeltonErrors{ ErrorType::Skleleton };
