#include "shader.h"

// ��ɫ�����캯�������ն����Ƭ����ɫ��·��
Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    std::string vertexCode; // �洢������ɫ������
    std::string fragmentCode; // �洢Ƭ����ɫ������
    std::ifstream vShaderFile; // ������ɫ���ļ���
    std::ifstream fShaderFile; // Ƭ����ɫ���ļ���

    // �����ļ����쳣����
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        // ����ɫ���ļ�
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);

        // ��ȡ�ļ����ݵ��ַ�����
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf(); // ��ȡ������ɫ��
        fShaderStream << fShaderFile.rdbuf(); // ��ȡƬ����ɫ��
        vShaderFile.close(); // �ر��ļ���
        fShaderFile.close(); // �ر��ļ���

        // ���ַ�����ת��Ϊ�ַ���
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl; // �ļ���ȡ����
    }

    const char* vShaderCode = vertexCode.c_str(); // ������ɫ������
    const char* fShaderCode = fragmentCode.c_str(); // Ƭ����ɫ������

    // ��ɫ������
    unsigned int vertex, fragment; // ��ɫ������
    int success; // ����״̬
    char infoLog[512]; // ������־

    // ���붥����ɫ��
    vertex = glCreateShader(GL_VERTEX_SHADER); // ����������ɫ��
    glShaderSource(vertex, 1, &vShaderCode, NULL); // ������ɫ��Դ����
    glCompileShader(vertex); // ������ɫ��
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success); // ��ȡ����״̬
    if (!success)
    {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog); // ��ȡ������־
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl; // ���������Ϣ
    };

    // ����Ƭ����ɫ��
    fragment = glCreateShader(GL_FRAGMENT_SHADER); // ����Ƭ����ɫ��
    glShaderSource(fragment, 1, &fShaderCode, NULL); // ������ɫ��Դ����
    glCompileShader(fragment); // ������ɫ��
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success); // ��ȡ����״̬
    if (!success)
    {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog); // ��ȡ������־
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl; // ���������Ϣ
    };

    // ������ɫ������
    id = glCreateProgram(); // �����������
    glAttachShader(id, vertex); // ���Ӷ�����ɫ��
    glAttachShader(id, fragment); // ����Ƭ����ɫ��
    glLinkProgram(id); // ���ӳ���
    glGetProgramiv(id, GL_LINK_STATUS, &success); // ��ȡ����״̬
    if (!success)
    {
        glGetProgramInfoLog(id, 512, NULL, infoLog); // ��ȡ������־
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl; // ���������Ϣ
    }

    // ɾ����ɫ������Ϊ���������ӵ�������
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

// ʹ����ɫ������
void Shader::use() {
    glUseProgram(id); // ������ɫ������
}

// ���ò����� uniform ����
void Shader::setBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value); // ���ò���ֵ
}

// �������� uniform ����
void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(id, name.c_str()), value); // ��������ֵ
}

// ���ø����� uniform ����
void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(id, name.c_str()), value); // ���ø���ֵ
}

// ����4x4����uniform����
void Shader::SetMat4(const std::string& name, glm::mat4 value) {
    glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, &value[0][0]);
}

// ����3D����uniform����
void Shader::SetVec3(const std::string& name, const glm::vec3& value) {
    glUniform3fv(glGetUniformLocation(id, name.c_str()), 1, &value[0]);
}