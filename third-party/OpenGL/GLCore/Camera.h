#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const glm::vec3 UP{ 0.0f, 1.0f, 0.0f };

class Camera
{
public:
	Camera() : 
		cameraDirection(glm::normalize(cameraPos - cameraTarget)), 
		cameraRight(glm::normalize(glm::cross(UP, cameraDirection))),
		cameraUp(glm::cross(cameraDirection, cameraRight)),
		view(glm::lookAt(cameraPos, cameraTarget, UP)) { }

	void updateTest();

	inline glm::mat4 getView() const
	{
		return view;
	}

	inline glm::mat4 getProjection() const
	{
		return proj;
	}

	inline glm::mat4 getViewProjection() const
	{
		return view;
	}

private:
	glm::vec3 cameraPos{ 0.0f, 0.0f, 3.0f };
	glm::vec3 cameraTarget{ 0.0f, 0.0f, 0.0f };
	glm::vec3 cameraDirection;
	glm::vec3 cameraRight;
	glm::vec3 cameraUp;

	glm::mat4 view;
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), 960.0f / 540.0f, -1.0f, 1.0f);

};