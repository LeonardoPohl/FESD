#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <functional>

const glm::vec3 UP{ 0.0f, 1.0f, 0.0f };

struct GLFWwindow;

// Default camera values
const float YAW = 0.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;

class Camera
{
public:
	explicit Camera(GLFWwindow *window);

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
		return proj * view;
	}

	void processKeyboardInput(float deltaTime = 0);
	void processMousePosUpdate(double xpos, double ypos);
	void processScroll(double xoffset, double yoffset);
	void updateImGui();
private:
	void updateCameraVectors();
	void updateView();
	void updateProjection();

	GLFWwindow *window;

	glm::vec3 Position	{ 0.0f, 0.0f, 0.0f };
	glm::vec3 Front {0.0f, 0.0f, -1.0f};
	glm::vec3 Right;
	glm::vec3 Up;

	glm::mat4 view;
	glm::mat4 proj;

	float pitch{ PITCH };
	float yaw{ YAW };
	float cameraSpeed{ SPEED };
	float sensitivity{ SENSITIVITY };

	float fov{ 45.0f };

	float lastX{ -1.0f };
	float lastY{ -1.0f };

	bool firstMouse{ true };
};