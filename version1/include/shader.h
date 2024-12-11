#pragma once

#include <glm/glm.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    Shader(const char* vertexPath, const char* fragmentPath);

    void use();
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void SetMat4(const std::string& name, glm::mat4 value);
    void SetVec3(const std::string& name, const glm::vec3& value);

    int getInt(const std::string& name) const;

    unsigned int id;
};