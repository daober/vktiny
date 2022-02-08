#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;
layout (location = 4) in vec3 inTangent;

layout (set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 viewPos;
} ubo;


layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outTangentViewPos;
layout (location = 2) out vec3 outTangentLightPos;
layout (location = 3) out vec3 outTangentFragPos;


void main() {

    vec3 outFragPos = vec3(ubo.model * vec4(inPosition, 1.0));
    outUV = inUV;

    mat3 normalMatrix = transpose(inverse(mat3(ubo.model)));
    vec3 T = normalize(normalMatrix * inTangent);
    vec3 N = normalize(normalMatrix * inNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    mat3 TBN = transpose(mat3(T, B, N));
    outTangentLightPos = TBN * ubo.lightPos;
    outTangentViewPos  = TBN * ubo.viewPos;
    outTangentFragPos  = TBN * outFragPos;

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
}