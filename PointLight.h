#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class PointLight
{
public:
	PointLight(const glm::vec3 color);

	glm::vec3 getColor() { return _color; }

private:
	glm::vec3 _color;

};

