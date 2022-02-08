#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;
layout (location = 4) in vec4 inTangent;

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 viewPos;
} ubo;

//layout(location = 0) out vec3 fragColor;
layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;
layout (location = 5) out vec4 outTangent;


void main() {
    outNormal = inNormal;
    outColor = inColor;
    outUV = inUV;
    outTangent = inTangent;
    outNormal = mat3(ubo.model) * inNormal;

    vec4 pos = ubo.model * vec4(inPosition, 1.0);
    outLightVec = ubo.lightPos - pos.xyz;
    outViewVec = ubo.viewPos - pos.xyz;

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
}