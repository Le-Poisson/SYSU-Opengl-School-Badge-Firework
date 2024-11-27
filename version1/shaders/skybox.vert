#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;

out vec3 TexCoords;

//setujemo aPos preko prosledjivanja i preko setMat4 funkcija dodajemo uniform vrednosti da bi odredili position

void main()
{
    TexCoords = aPos;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}