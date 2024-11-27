#include "camera.h"

// 静态变量初始化
float Camera::currentFrame = 0.0f; // 当前帧时间
float Camera::deltaTime = 0.0f; // 帧间时间
float Camera::lastFrame = 0.0f; // 上一帧时间

// 更新帧间时间
void Camera::updateDeltaTime() {
	currentFrame = glfwGetTime(); // 获取当前时间
	deltaTime = currentFrame - lastFrame; // 计算帧间时间
	lastFrame = currentFrame; // 更新上一帧时间
}

// 默认构造函数，初始化位置为原点
Camera::Camera()
	: Camera(glm::vec3(0.0f, 0.0f, 0.0f))
{}

// 带位置的构造函数
Camera::Camera(glm::vec3 position) :
	position(position), // 设置相机位置
	direction(0.0f, 0.1f, 1.0f), // 设置初始方向
	UP(0.0f, 1.0f, 0.0f), // 定义向上方向
	speed(25.0f) // 设置移动速度
{}

// 获取相机位置
glm::vec3 Camera::getPosition() const
{
	return position; // 返回当前位置
}

// 获取相机方向
glm::vec3 Camera::getDirection() const
{
	return direction; // 返回当前方向
}

// 获取帧间时间
float Camera::getDeltaTime() {
	return deltaTime; // 返回帧间时间
}

// 设置相机移动速度
void Camera::setSpeed(float newSpeed)
{
	this->speed = newSpeed; // 更新速度
}

// 获取相机实际移动速度（考虑帧间时间）
float Camera::getSpeed() const {
	return speed * deltaTime; // 返回实际速度
}

// 旋转相机方向
void Camera::rotate(float yaw, float pitch)
{
	// 计算方向向量
	direction.x = cos(glm::radians(yaw));
	direction.z = sin(glm::radians(yaw));
	direction.y = sin(glm::radians(pitch));

	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch)); // 更新X方向
	direction.y = sin(glm::radians(pitch)); // 更新Y方向
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch)); // 更新Z方向
}

// 向上移动相机
void Camera::moveUp(float axis)
{
	position += UP * getSpeed() * axis; // 根据速度和输入轴向上移动
}

// 向左移动相机
void Camera::moveLeft(float axis)
{
	position += glm::normalize(glm::cross(direction, UP)) * getSpeed() * axis; // 计算左方向并移动
}

// 向前移动相机
void Camera::moveForward(float axis)
{
	position += direction * getSpeed() * axis; // 根据方向移动
}

// 获取世界到视图矩阵
glm::mat4 Camera::getWorldToViewMatrix() const
{
	return glm::lookAt(position, position + direction, UP); // 生成视图矩阵
}

glm::mat4 Camera::to_mat4() {
	glm::vec3 target = position + direction; // Look in the direction the camera is facing

	return glm::lookAt(position, target, UP); // Create the view matrix
}