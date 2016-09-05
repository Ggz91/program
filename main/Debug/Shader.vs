#version 330 core

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 Nor;

out vec2 fragUV;
out vec3 vPos;
out vec3 vNor;
out vec3 vEyeDirection;
out vec3 vLightDirection;
out vec3 wPos;

uniform mat4 MVP;
uniform mat4 V;
uniform mat4 M;
uniform vec3 LightPos;


void main()
{
	gl_Position = MVP * vec4(Pos, 1);
	fragUV = UV;

	vPos = ( V * M * vec4(Pos, 1) ).xyz;
	vNor = ( V * M * vec4(Nor, 0) ).xyz;
	wPos = ( M * vec4(Pos, 1)).xyz;

	vEyeDirection = vec3(0, 0, 0) - vPos;
	
	vec3 vLightPostion = (V * vec4(LightPos, 1)).xyz;

	vLightDirection =  vLightPostion + vEyeDirection;
}