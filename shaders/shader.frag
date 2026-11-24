#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec4 fragLightPosition;
layout(location = 0) out vec4 outColor;

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
layout(set = 0, binding = 1) uniform sampler2D albedoTexture;
layout(set = 0, binding = 2) uniform sampler2DShadow shadowMap;

void main() {
    vec3 albedo = texture(albedoTexture, fragUV).rgb;
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(scene.cameraPosition.xyz - fragPosition);
    vec3 L = normalize(-scene.lightDirection.xyz);
    vec3 color = albedo * scene.ambientColorIntensity.rgb *
                 scene.ambientColorIntensity.a;
    float diffuse = max(dot(N, L), 0.0);
    if (diffuse > 0.0) {
        vec3 shadowCoord = fragLightPosition.xyz / fragLightPosition.w;
        shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;
        float visibility = 1.0;
        if (all(greaterThanEqual(shadowCoord, vec3(0.0))) &&
            all(lessThanEqual(shadowCoord, vec3(1.0)))) {
            visibility = texture(shadowMap, shadowCoord);
        }
        vec3 H = normalize(L + V);
        float specular = pow(max(dot(N, H), 0.0),
                             scene.specularColorShininess.a);
        color += visibility * scene.lightColorIntensity.rgb * scene.lightColorIntensity.a *
                 (albedo * diffuse + scene.specularColorShininess.rgb * specular);
    }
    outColor = vec4(color, 1.0);
}
