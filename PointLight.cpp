#include "PointLight.h"

PointLight::PointLight(const Color& color, const Position& position, const Attenuation& attenuation)
	:_color(color), _position(position), _attenuation(attenuation) {}

float PointLight::distance(Position fragment)
{
	return glm::distance(_position, fragment);
}

Color PointLight::calcAddColor(Position fragment, Direction normal)
{
	// TODO.
}
