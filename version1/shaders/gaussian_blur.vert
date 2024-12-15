#version 460

layout(location = 0) in vec3 aPos;      // 位置
layout(location = 1) in vec2 aTexCoords; // 纹理坐标

out vec2 TexCoords; // 传递给片段着色器的纹理坐标

void main()
{
    TexCoords = aTexCoords; // 将纹理坐标传递给片段着色器
    gl_Position = vec4(aPos, 1.0); // 设置顶点的最终位置
}