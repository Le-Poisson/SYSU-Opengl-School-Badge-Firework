#pragma once
#define GLM_ENABLE_EXPERIMENTAL

#include "Constant.h"
#include "shader.h"

using Color = glm::vec3;
using Position = glm::vec3;
using Attenuation = glm::vec3;
using Direction = glm::vec3;

class PointLight
{
public:
	PointLight();
	PointLight(const Color& color, const Position& position, const Attenuation& attenuation, const float life);
	~PointLight() = default;

	#pragma region Get&Set
	Color getColor() { return _color; }
	Position getPosition() { return _position; }
	Attenuation getAttenuation() { return _attenuation; }
	float getLife() { return _life; }
	// Unable to Set. 
	#pragma endregion

	float distance(const Position& fragment); // Calculate DISTANCE between light & fragment.
	Color calcAddColor(const Position& fragment, const Direction& normal); // Calculate COLOR should be ADDED on the fragment.
	bool updateLife(const float decrease);

	bool addToShader(const std::shared_ptr<Shader> shader, const int index);
	bool deleteFromShader(const std::shared_ptr<Shader> shader, const int index);

public:
	Color _color;
	Position _position;
	Attenuation _attenuation;
	float _life;

};

