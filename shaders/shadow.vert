#version 450

layout(location = 0) in vec3 inPosition;

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
    gl_Position = scene.lightViewProjection * scene.model * vec4(inPosition, 1.0);
}
