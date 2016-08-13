#version 330 core
layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 UV;

uniform mat4 MVP;
uniform mat4 V;
uniform mat4 M;

out vec2 fragUV;

void main()
{
	gl_Position = MVP * vec4(Pos, 1);
	fragUV = UV;
}