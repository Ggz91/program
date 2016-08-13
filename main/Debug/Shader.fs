#version 330 core

in vec2 fragUV;
out vec3 color;

uniform sampler2D textSampler;


void main()
{
    color = texture(textSampler, fragUV).rgb;
}