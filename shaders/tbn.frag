#version 450

layout (set = 1, binding = 0) uniform sampler2D samplerColorMap;
layout (set = 1, binding = 1) uniform sampler2D samplerNormalMap;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inTangentViewPos;
layout (location = 2) in vec3 inTangentLightPos;
layout (location = 3) in vec3 inTangentFragPos;


layout (location = 0) out vec4 outFragColor;

void main() {

    vec3 normal = texture(samplerNormalMap, inUV).rgb;
    normal = normalize(normal * 2.0 - 1.0);

    //get diffuse color
    vec3 color = texture(samplerColorMap, inUV).rgb;

    //ambient
    vec3 ambient = 0.1 * color;
    
    //diffuse
    vec3 lightDir = normalize(inTangentLightPos - inTangentFragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;

    //specular
    vec3 viewDir = normalize(inTangentViewPos - inTangentFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    vec3 specular = vec3(0.2) * spec;
    outFragColor = vec4(ambient + color.rgb + specular, 1.0);
}