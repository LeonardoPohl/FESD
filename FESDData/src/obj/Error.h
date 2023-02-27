#pragma once
#include <string>
#include <filesystem>

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

	void Slider(int *err_id) {
		ImGui::SliderInt("", err_id, 0, errors.size(), errors[*err_id].name.c_str());
	}

	Error operator[](int i) {
		return errors[i];
	}

	std::vector<Error> errors;
	ErrorType errorType;
};

static Errors JointErrors{ ErrorType::Joint };
static Errors SkeltonErrors{ ErrorType::Skleleton };
