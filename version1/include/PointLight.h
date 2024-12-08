#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

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

	float distance(Position fragment); // Calculate DISTANCE between light & fragment.
	Color calcAddColor(Position fragment, Direction normal); // Calculate COLOR should be ADDED on the fragment.

public:
	Color _color;
	Position _position;
	Attenuation _attenuation;

};

