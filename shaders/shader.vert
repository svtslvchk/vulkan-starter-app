#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragUV;
layout(location = 3) out vec4 fragLightPosition;

layout(set = 0, binding = 0, std140) uniform Uniforms {
    mat4 model;
    mat4 viewProjection;
    mat4 lightViewProjection;
    vec4 cameraPosition;
    vec4 lightDirection;
    vec4 lightColorIntensity;
    vec4 ambientColorIntensity;
    vec4 specularColorShininess;
} scene;

void main() {
    vec4 worldPosition = scene.model * vec4(inPosition, 1.0);
    gl_Position = scene.viewProjection * worldPosition;
    fragPosition = worldPosition.xyz;
    fragNormal = normalize(transpose(inverse(mat3(scene.model))) * inNormal);
    fragUV = inUV;
    fragLightPosition = scene.lightViewProjection * worldPosition;
}
