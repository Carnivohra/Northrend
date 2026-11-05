#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 1, binding = 0) uniform DirectionalLight {
    vec4 directionAmbient;
    vec4 colorIntensity;
} light;

layout(set = 2, binding = 0) uniform sampler2D textures[64];

layout(location = 0) in vec3 worldNormal;
layout(location = 1) in vec2 terrainCoordinates;
layout(location = 2) flat in uint terrainTexture;
layout(location = 0) out vec4 fragmentColor;

void main() {
    const vec2 tiledCoordinates = terrainCoordinates * 8.0;
    const vec3 albedo = texture(textures[nonuniformEXT(terrainTexture)], tiledCoordinates).rgb;
    const float diffuse = max(dot(normalize(worldNormal), -light.directionAmbient.xyz), 0.0);
    const float illumination = light.directionAmbient.w + diffuse * light.colorIntensity.w;
    fragmentColor = vec4(albedo * light.colorIntensity.rgb * illumination, 1.0);
}
