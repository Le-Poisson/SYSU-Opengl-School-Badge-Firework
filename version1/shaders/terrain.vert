#version 420 core

layout (location = 0) in vec3 aPosition;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec2 TexCoord;
out vec3 WorldPos;

float mapRange(float value, float min1, float max1, float min2, float max2) {
	return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

void main() {
	
	float width = 1756;  // height map width
	float height = 2624; // height map height	
	
	// Calculate texture coordinates by mapping from vertex position to 0 to 1 range,
	// isto kao u kodu ide nam od -1756/2 i -height/2 do + toliko zato radimo ovaj norm i saljemo koordinate i word pos
	float y = mapRange(aPosition.x, -width * 0.5, width * 0.5, 0, 1);
	float x = mapRange(aPosition.z, -height * 0.5, height * 0.5, 0, 1);
	TexCoord = vec2(x,y);

	WorldPos = (model * vec4(aPosition, 1.0)).xyz;
	gl_Position = projection * view * model * vec4(aPosition, 1.0);
}