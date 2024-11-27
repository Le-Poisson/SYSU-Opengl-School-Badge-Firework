#version 420 core

layout (location = 0) out vec4 FragColor;
layout (binding = 0) uniform samplerCube cubeMap;

in vec3 TexCoords;

// na tutorialu kaze samo uzmem mapu i ove koordinate i mogu da gadjam koji deo skyboxa gledam
void main() {		
    FragColor.rgb = texture(cubeMap, TexCoords).rgb;
    FragColor.a = 1.0;
}
