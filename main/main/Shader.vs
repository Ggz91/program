#version 330 core
layout(location = 0) in vec3 Pos;

uniform mat4 MVP;
uniform mat4 V;
uniform mat4 M;

void main()
{
	gl_Position = MVP * vec4(Pos, 1);
}