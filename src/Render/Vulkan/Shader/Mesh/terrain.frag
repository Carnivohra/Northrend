#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 1, binding = 0) uniform DirectionalLight {
    vec4 directionAmbient;
    vec4 colorIntensity;
} light;

layout(set = 2, binding = 0) uniform sampler2D textures[64];
layout(set = 2, binding = 1) uniform sampler2DArray alphaMaps;

layout(push_constant) uniform TerrainDetail {
    float textureMipBias;
    float alphaMipBias;
} detail;

layout(location = 0) in vec3 worldNormal;
layout(location = 1) in vec2 terrainCoordinates;
layout(location = 2) flat in uvec4 terrainTextures;
layout(location = 3) flat in uint terrainMaterial;
layout(location = 0) out vec4 fragmentColor;

void main() {
    const uint alphaLayer = terrainMaterial & 0xFF;
    const uint layerCount = (terrainMaterial >> 8) + 1;
    const vec2 tiledCoordinates = terrainCoordinates * 8.0;
    vec3 albedo = texture(textures[nonuniformEXT(terrainTextures.x)], tiledCoordinates, detail.textureMipBias).rgb;

    if (layerCount > 1) {
        const vec2 alphaCoordinates = clamp(terrainCoordinates, vec2(0.5 / 64.0), vec2(1.0 - 0.5 / 64.0));
        const vec3 alpha = texture(alphaMaps, vec3(alphaCoordinates, float(alphaLayer)), detail.alphaMipBias).rgb;
        const float baseWeight = max(1.0 - alpha.r - alpha.g - alpha.b, 0.0);
        albedo *= baseWeight;
        albedo +=
            texture(textures[nonuniformEXT(terrainTextures.y)], tiledCoordinates, detail.textureMipBias).rgb * alpha.r;

        if (layerCount > 2) {
            albedo +=
                texture(textures[nonuniformEXT(terrainTextures.z)], tiledCoordinates, detail.textureMipBias).rgb *
                alpha.g;
        }

        if (layerCount > 3) {
            albedo +=
                texture(textures[nonuniformEXT(terrainTextures.w)], tiledCoordinates, detail.textureMipBias).rgb *
                alpha.b;
        }
    }

    const float diffuse = max(dot(normalize(worldNormal), -light.directionAmbient.xyz), 0.0);
    const float illumination = light.directionAmbient.w + diffuse * light.colorIntensity.w;
    fragmentColor = vec4(albedo * light.colorIntensity.rgb * illumination, 1.0);
}
