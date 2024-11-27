#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <iostream>

class Camera
{
public:
	Camera();
	Camera(glm::vec3 position);

	glm::mat4 getWorldToViewMatrix() const;
	glm::vec3 getPosition() const;
	glm::vec3 getDirection() const;

	void moveUp(float axis);
	void moveForward(float axis);
	void moveLeft(float axis);
	void rotate(float yaw, float pitch);

	float getSpeed() const;
	void setSpeed(float newSpeed);
	static float getDeltaTime();
	static void updateDeltaTime();
	glm::mat4 to_mat4();

private:
	glm::vec3 position;
	glm::vec3 direction;
	const glm::vec3 UP;
	float speed;

	static float currentFrame; // 当前帧时间
	static float deltaTime; // 帧间时间
	static float lastFrame; // 上一帧时间
};
