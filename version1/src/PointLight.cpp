#include <string>

#include "PointLight.h"

PointLight::PointLight()
    :_color(Color(0.0f, 0.0f, 0.0f)), _position(Position(500.0f)), _attenuation(Attenuation(1.0f, 0.007f, 0.0002f)), _life(100) {}

PointLight::PointLight(const Color& color, const Position& position, const Attenuation& attenuation, const float life)
	:_color(color), _position(position), _attenuation(attenuation), _life(life) {}

float PointLight::distance(const Position& fragment)
{
	return glm::distance(_position, fragment);
}

Color PointLight::calcAddColor(const Position& fragment, const Direction& normal)
{
    Direction lightDir = glm::normalize(_position - fragment);
    float dist = glm::length(_position - fragment);
    float attenuation = 1.0f / (_attenuation.x + _attenuation.y * dist + _attenuation.z * dist * dist);
    float diff = glm::max(glm::dot(normal, lightDir), 0.0f);
    return _color * diff * attenuation;
}

bool PointLight::updateLife(const float decrease)
{ // return true if it still alive.
    _life = _life - decrease;
    return (_life > 0);
}

bool PointLight::addToShader(const std::shared_ptr<Shader> shader, const int index)
{
    if (index >= MAX_LIGHTS || index < 0) return false;
    shader->use();

    shader->SetVec3("pointLights[" + std::to_string(index) + "].position", _position); // 设置位置
    shader->SetVec3("pointLights[" + std::to_string(index) + "].color", _color); // 设置颜色
    shader->SetVec3("pointLights[" + std::to_string(index) + "].attenuation", _attenuation); // 设置衰减

    return true;
}

bool PointLight::deleteFromShader(const std::shared_ptr<Shader> shader, const int index)
{
    if (index < 0) return false;
    shader->use();

    shader->SetVec3("pointLights[" + std::to_string(index) + "].color", glm::vec3(0.0f)); // 设置颜色

    return true;
}
