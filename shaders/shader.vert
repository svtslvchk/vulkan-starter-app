#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosition;
layout(location = 2) out vec3 fragNormal;

layout(set = 0, binding = 0, std140) uniform SceneUniforms {
    mat4 viewProjection;
    vec4 cameraPosition;
    vec4 lightDirection;
    vec4 lightColorIntensity;
    vec4 ambientColorIntensity;
    uvec4 lightCounts;
} scene;

layout(set = 1, binding = 0, std140) uniform ModelUniforms {
    mat4 model;
    vec4 objectColor;
    vec4 specularColorShininess;
} objectData;

void main() {
    vec4 worldPosition = objectData.model * vec4(inPosition, 1.0);
    gl_Position = scene.viewProjection * worldPosition;
    fragPosition = worldPosition.xyz;
    mat3 normalMatrix = transpose(inverse(mat3(objectData.model)));
    fragNormal = normalize(normalMatrix * inNormal);
    fragColor = inColor * objectData.objectColor.rgb;
}
