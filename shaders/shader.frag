#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosition;
layout(location = 2) in vec3 fragNormal;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0, std140) uniform SceneUniforms {
    mat4 viewProjection;
    vec4 cameraPosition;
    vec4 lightDirection;
    vec4 lightColorIntensity;
    vec4 ambientColorIntensity;
    uvec4 lightCounts;
} scene;

struct PointLight {
    vec4 positionIntensity;
    vec4 color;
};
layout(set = 0, binding = 1, std430) readonly buffer PointLights {
    PointLight lights[];
} points;

struct SpotLight {
    vec4 positionIntensity;
    vec4 directionInnerCos;
    vec4 colorOuterCos;
};
layout(set = 0, binding = 2, std430) readonly buffer SpotLights {
    SpotLight lights[];
} spots;

layout(set = 1, binding = 0, std140) uniform ModelUniforms {
    mat4 model;
    vec4 objectColor;
    vec4 specularColorShininess;
} objectData;

vec3 blinnPhong(vec3 N, vec3 V, vec3 L, vec3 radiance) {
    float diffuse = max(dot(N, L), 0.0);
    if (diffuse <= 0.0) return vec3(0.0);
    vec3 H = normalize(L + V);
    float specular = pow(max(dot(N, H), 0.0),
                         objectData.specularColorShininess.a);
    return radiance * (fragColor * diffuse +
                       objectData.specularColorShininess.rgb * specular);
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(scene.cameraPosition.xyz - fragPosition);
    vec3 color = fragColor * scene.ambientColorIntensity.rgb *
                 scene.ambientColorIntensity.a;

    vec3 directionalL = normalize(-scene.lightDirection.xyz);
    color += blinnPhong(N, V, directionalL,
                        scene.lightColorIntensity.rgb * scene.lightColorIntensity.a);

    for (uint i = 0u; i < scene.lightCounts.x; ++i) {
        vec3 offset = points.lights[i].positionIntensity.xyz - fragPosition;
        float distanceSquared = dot(offset, offset);
        vec3 L = offset * inversesqrt(max(distanceSquared, 0.0001));
        vec3 radiance = points.lights[i].color.rgb *
                        points.lights[i].positionIntensity.a /
                        max(distanceSquared, 0.01);
        color += blinnPhong(N, V, L, radiance);
    }

    for (uint i = 0u; i < scene.lightCounts.y; ++i) {
        vec3 offset = spots.lights[i].positionIntensity.xyz - fragPosition;
        float distanceSquared = dot(offset, offset);
        vec3 L = offset * inversesqrt(max(distanceSquared, 0.0001));
        // Из источника к поверхности: -L. Внутри конуса cos угла больше.
        float cosTheta = dot(-L, spots.lights[i].directionInnerCos.xyz);
        float innerCos = spots.lights[i].directionInnerCos.a;
        float outerCos = spots.lights[i].colorOuterCos.a;
        float cone = innerCos > outerCos + 0.00001
            ? smoothstep(outerCos, innerCos, cosTheta)
            : step(outerCos, cosTheta);
        vec3 radiance = spots.lights[i].colorOuterCos.rgb *
                        spots.lights[i].positionIntensity.a * cone /
                        max(distanceSquared, 0.01);
        color += blinnPhong(N, V, L, radiance);
    }

    outColor = vec4(color, 1.0);
}
