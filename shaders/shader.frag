#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosition;
layout(location = 2) in vec3 fragNormal;
layout(location = 0) out vec4 outColor;

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

vec3 blinnPhong(vec3 N, vec3 V, vec3 L, vec3 radiance) {
    float diffuse = max(dot(N, L), 0.0);
    if (diffuse <= 0.0) return vec3(0.0);
    vec3 H = normalize(L + V);
    float specular = pow(max(dot(N, H), 0.0),
                         scene.specularColorShininess.a);
    return radiance * (fragColor * diffuse +
                       scene.specularColorShininess.rgb * specular);
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(scene.cameraPosition.xyz - fragPosition);
    vec3 color = fragColor * scene.ambientColorIntensity.rgb *
                 scene.ambientColorIntensity.a;

    vec3 directionalL = normalize(-scene.lightDirection.xyz);
    color += blinnPhong(N, V, directionalL,
                        scene.lightColorIntensity.rgb * scene.lightColorIntensity.a);

    outColor = vec4(color, 1.0);
}
