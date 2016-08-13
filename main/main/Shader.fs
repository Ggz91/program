#version 330 core

in vec2 fragUV;
in vec3 vPos;
in vec3 vNor;
in vec3 vEyeDirection;
in vec3 vLightDirection;
in vec3 wPos;

out vec3 color;

uniform sampler2D textSampler;
uniform vec3 LightPos;

void main()
{
    vec3 LightColor = vec3(1, 1, 1);
    float LightPower = 50.0f;

    vec3 MaterialDiffuseColor = texture(textSampler, fragUV).rgb;
    vec3 MaterialAmbientColor = vec3(0.2, 0.2, 0.2) * MaterialDiffuseColor;
    vec3 MaterialSpecularColor = vec3(0.4, 0.4, 0.4);

    float fDis = length(LightPos - wPos);

    vec3 nor = normalize(vNor);
    vec3 lig = normalize(vLightDirection);

    float cosTheta = clamp( dot(nor,lig), 0, 1);
    vec3 eye = normalize(vEyeDirection);
    vec3 ref = reflect(-lig,nor);

    float cosAlpha = clamp(dot(eye, ref), 0, 1);

    color = MaterialAmbientColor +
            MaterialDiffuseColor * LightColor * LightPower * cosTheta / (fDis * fDis) + 
            MaterialSpecularColor * LightColor * LightPower * pow(cosAlpha, 5) / (fDis * fDis);

}