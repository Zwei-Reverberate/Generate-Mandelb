#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>


enum Camera_Movement
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class Camera
{
public:
	// Camera properties
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;
	float Yaw;
	float Pitch;
	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;

	Camera(glm::vec3 _position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 _up = glm::vec3(0.0f, 1.0f, 0.0f),
		float _yaw = YAW, float _pitch = PITCH);
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float _yaw, float _pitch);
	glm::mat4 GetViewMatrix();
	void ProcessKeyboard(Camera_Movement direction, float deltaTime);
	void ProcessMouseMovement(double xoffset, double yoffset, GLboolean constrainPitch = true);
	void ProcessMouseScroll(double yoffset);

private:
	void updateCameraVectors();
};

