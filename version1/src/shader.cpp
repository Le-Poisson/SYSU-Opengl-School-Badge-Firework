#include "shader.h"

// 着色器构造函数，接收顶点和片段着色器路径
Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    std::string vertexCode; // 存储顶点着色器代码
    std::string fragmentCode; // 存储片段着色器代码
    std::ifstream vShaderFile; // 顶点着色器文件流
    std::ifstream fShaderFile; // 片段着色器文件流

    // 设置文件流异常处理
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        // 打开着色器文件
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);

        // 读取文件内容到字符串流
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf(); // 读取顶点着色器
        fShaderStream << fShaderFile.rdbuf(); // 读取片段着色器
        vShaderFile.close(); // 关闭文件流
        fShaderFile.close(); // 关闭文件流

        // 将字符串流转换为字符串
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl; // 文件读取错误
    }

    const char* vShaderCode = vertexCode.c_str(); // 顶点着色器代码
    const char* fShaderCode = fragmentCode.c_str(); // 片段着色器代码

    // 着色器编译
    unsigned int vertex, fragment; // 着色器对象
    int success; // 编译状态
    char infoLog[512]; // 错误日志

    // 编译顶点着色器
    vertex = glCreateShader(GL_VERTEX_SHADER); // 创建顶点着色器
    glShaderSource(vertex, 1, &vShaderCode, NULL); // 设置着色器源代码
    glCompileShader(vertex); // 编译着色器
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success); // 获取编译状态
    if (!success)
    {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog); // 获取错误日志
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl; // 输出错误信息
    };

    // 编译片段着色器
    fragment = glCreateShader(GL_FRAGMENT_SHADER); // 创建片段着色器
    glShaderSource(fragment, 1, &fShaderCode, NULL); // 设置着色器源代码
    glCompileShader(fragment); // 编译着色器
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success); // 获取编译状态
    if (!success)
    {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog); // 获取错误日志
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl; // 输出错误信息
    };

    // 创建着色器程序
    id = glCreateProgram(); // 创建程序对象
    glAttachShader(id, vertex); // 附加顶点着色器
    glAttachShader(id, fragment); // 附加片段着色器
    glLinkProgram(id); // 链接程序
    glGetProgramiv(id, GL_LINK_STATUS, &success); // 获取链接状态
    if (!success)
    {
        glGetProgramInfoLog(id, 512, NULL, infoLog); // 获取错误日志
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl; // 输出错误信息
    }

    // 删除着色器，因为它们已链接到程序中
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

// 使用着色器程序
void Shader::use() {
    glUseProgram(id); // 激活着色器程序
}

// 设置布尔型 uniform 变量
void Shader::setBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value); // 设置布尔值
}

// 设置整型 uniform 变量
void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(id, name.c_str()), value); // 设置整型值
}

// 设置浮点型 uniform 变量
void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(id, name.c_str()), value); // 设置浮点值
}

// 设置4x4矩阵uniform变量
void Shader::SetMat4(const std::string& name, glm::mat4 value) {
    glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, &value[0][0]);
}

// 设置3D向量uniform变量
void Shader::SetVec3(const std::string& name, const glm::vec3& value) {
    glUniform3fv(glGetUniformLocation(id, name.c_str()), 1, &value[0]);
}