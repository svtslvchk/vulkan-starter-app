#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosition;
layout(location = 2) out vec3 fragNormal;

layout(set = 0, binding = 0, std140) uniform Uniforms {
    mat4 model;
    mat4 viewProjection;
    vec4 cameraPosition;
    vec4 lightDirection;
    vec4 lightColorIntensity;
    vec4 ambientColorIntensity;
    vec4 objectColor;
    vec4 specularColorShininess;
} scene;

void main() {
    vec4 worldPosition = scene.model * vec4(inPosition, 1.0);
    gl_Position = scene.viewProjection * worldPosition;
    fragPosition = worldPosition.xyz;
    mat3 normalMatrix = transpose(inverse(mat3(scene.model)));
    fragNormal = normalize(normalMatrix * inNormal);
    fragColor = inColor * scene.objectColor.rgb;
}
