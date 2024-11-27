#include "camera.h"

// ��̬������ʼ��
float Camera::currentFrame = 0.0f; // ��ǰ֡ʱ��
float Camera::deltaTime = 0.0f; // ֡��ʱ��
float Camera::lastFrame = 0.0f; // ��һ֡ʱ��

// ����֡��ʱ��
void Camera::updateDeltaTime() {
	currentFrame = glfwGetTime(); // ��ȡ��ǰʱ��
	deltaTime = currentFrame - lastFrame; // ����֡��ʱ��
	lastFrame = currentFrame; // ������һ֡ʱ��
}

// Ĭ�Ϲ��캯������ʼ��λ��Ϊԭ��
Camera::Camera()
	: Camera(glm::vec3(0.0f, 0.0f, 0.0f))
{}

// ��λ�õĹ��캯��
Camera::Camera(glm::vec3 position) :
	position(position), // �������λ��
	direction(0.0f, 0.1f, 1.0f), // ���ó�ʼ����
	UP(0.0f, 1.0f, 0.0f), // �������Ϸ���
	speed(25.0f) // �����ƶ��ٶ�
{}

// ��ȡ���λ��
glm::vec3 Camera::getPosition() const
{
	return position; // ���ص�ǰλ��
}

// ��ȡ�������
glm::vec3 Camera::getDirection() const
{
	return direction; // ���ص�ǰ����
}

// ��ȡ֡��ʱ��
float Camera::getDeltaTime() {
	return deltaTime; // ����֡��ʱ��
}

// ��������ƶ��ٶ�
void Camera::setSpeed(float newSpeed)
{
	this->speed = newSpeed; // �����ٶ�
}

// ��ȡ���ʵ���ƶ��ٶȣ�����֡��ʱ�䣩
float Camera::getSpeed() const {
	return speed * deltaTime; // ����ʵ���ٶ�
}

// ��ת�������
void Camera::rotate(float yaw, float pitch)
{
	// ���㷽������
	direction.x = cos(glm::radians(yaw));
	direction.z = sin(glm::radians(yaw));
	direction.y = sin(glm::radians(pitch));

	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch)); // ����X����
	direction.y = sin(glm::radians(pitch)); // ����Y����
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch)); // ����Z����
}

// �����ƶ����
void Camera::moveUp(float axis)
{
	position += UP * getSpeed() * axis; // �����ٶȺ������������ƶ�
}

// �����ƶ����
void Camera::moveLeft(float axis)
{
	position += glm::normalize(glm::cross(direction, UP)) * getSpeed() * axis; // ���������ƶ�
}

// ��ǰ�ƶ����
void Camera::moveForward(float axis)
{
	position += direction * getSpeed() * axis; // ���ݷ����ƶ�
}

// ��ȡ���絽��ͼ����
glm::mat4 Camera::getWorldToViewMatrix() const
{
	return glm::lookAt(position, position + direction, UP); // ������ͼ����
}

glm::mat4 Camera::to_mat4() {
	glm::vec3 target = position + direction; // Look in the direction the camera is facing

	return glm::lookAt(position, target, UP); // Create the view matrix
}