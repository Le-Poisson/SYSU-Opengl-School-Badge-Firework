#include <string>
#include "PointLight.h"

PointLight::PointLight(const Color& color, const Position& position, const Attenuation& attenuation)
	:_color(color), _position(position), _attenuation(attenuation) {}

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

bool PointLight::addToShader(Shader& shader, const int index)
{
    int numPointLights = shader.getInt("numPointLights");
    if (numPointLights >= 10) return false;
    shader.SetVec3("pointLights[" + std::to_string(index) + "].position", _position); // 设置位置
    shader.SetVec3("pointLights[" + std::to_string(index) + "].color", _color); // 设置颜色
    shader.SetVec3("pointLights[" + std::to_string(index) + "].attenuation", _attenuation); // 设置衰减

    // 增加点光源数量
    shader.setInt("numPointLights", numPointLights + 1);
    return true;
}
