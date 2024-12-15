#version 460

layout(location = 0) in vec3 aPos;      // λ��
layout(location = 1) in vec2 aTexCoords; // ��������

out vec2 TexCoords; // ���ݸ�Ƭ����ɫ������������

void main()
{
    TexCoords = aTexCoords; // ���������괫�ݸ�Ƭ����ɫ��
    gl_Position = vec4(aPos, 1.0); // ���ö��������λ��
}