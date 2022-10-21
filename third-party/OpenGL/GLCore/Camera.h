#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <functional>

const glm::vec3 UP{ 0.0f, 1.0f, 0.0f };

struct GLFWwindow;

class Camera
{
public:
	Camera(GLFWwindow *window);

	inline glm::mat4 getView() const
	{
		return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	}

	inline glm::mat4 getProjection() const
	{
		return proj;
	}

	inline glm::mat4 getViewProjection() const
	{
		return view * proj;
	}

	void processKeyboardInput(float deltaTime = 0);
	void processMousePosUpdate(double xpos, double ypos);
	void processScroll(double xoffset, double yoffset);
private:
	void updateCameraVectors();
	void updateView();
	void updateProjection();

	GLFWwindow *window;

	glm::vec3 cameraPos	{ 0.0f, 0.0f, 3.0f };
	glm::vec3 cameraFront {0.0f, 0.0f, -1.0f};

	glm::vec3 cameraTarget{ 0.0f, 0.0f, 0.0f };
	glm::vec3 cameraDirection;
	glm::vec3 cameraRight;
	glm::vec3 cameraUp;

	glm::mat4 view;
	glm::mat4 proj;

	float pitch{ 0.0f };
	float yaw{ 0.0f };

	float cameraSpeed{ 0.05f };

	float lastX{ -1.0f };
	float lastY{ -1.0f };

	bool firstMouse{ true };
	float sensitivity{ 0.1f };

	float fov{ 40.0f };
};