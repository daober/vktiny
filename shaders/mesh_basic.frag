#version 450

layout (set = 1, binding = 0) uniform sampler2D samplerColorMap;
layout (set = 1, binding = 1) uniform sampler2D samplerNormalMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;
layout (location = 5) in vec4 inTangent;

layout (location = 0) out vec4 outFragColor;

void main() {
    vec3 normal = texture(samplerNormalMap, inUV).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    vec4 color = texture(samplerColorMap, inUV) * vec4(inColor, 1.0);

    //TBN
    vec3 N = normalize(inNormal);
    vec3 T = normalize(inTangent.xyz);
    vec3 B = cross(inNormal, inTangent.xyz) * inTangent.w;
    mat3 TBN = mat3(T, B, N);

    const float ambient = 0.1f;
    vec3 L = normalize(inLightVec);
    vec3 V = normalize(inViewVec);
    vec3 R = reflect(-L, N);

    vec3 diffuse = max(dot(N, L), ambient).rrr;
    float specular = pow(max(dot(R, V), 0.0), 32.0);

    outFragColor = vec4(diffuse * color.rgb + specular, color.a);
}