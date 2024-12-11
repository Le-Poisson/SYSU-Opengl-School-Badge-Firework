#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class Shader;

using Color = glm::vec3;
using Position = glm::vec3;
using Attenuation = glm::vec3;
using Direction = glm::vec3;

class PointLight
{
public:
	PointLight(const Color& color, const Position& position, const Attenuation& attenuation);
	~PointLight() = default;

	#pragma region Get&Set
	Color getColor() { return _color; }
	Position getPosition() { return _position; }
	Attenuation getAttenuation() { return _attenuation; }
	// Unable to Set. 
	#pragma endregion

	float distance(const Position& fragment); // Calculate DISTANCE between light & fragment.
	Color calcAddColor(const Position& fragment, const Direction& normal); // Calculate COLOR should be ADDED on the fragment.
	bool addToShader(Shader& shader, const int index);

public:
	Color _color;
	Position _position;
	Attenuation _attenuation;

};

