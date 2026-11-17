#version 450

layout(set = 1, binding = 0) uniform DirectionalLight {
    vec4 directionAmbient;
    vec4 colorIntensity;
} light;

layout(set = 2, binding = 0) uniform sampler2D textures[64];

layout(push_constant) uniform Material {
    float textureMipBias;
    float alphaMipBias;
    uint shader;
    uint blendMode;
    uint flags;
    uint textureCount;
} material;

layout(location = 0) in vec2 objectCoordinates;
layout(location = 1) in vec3 worldNormal;
layout(location = 0) out vec4 fragmentColor;

void main() {
    vec4 albedo = texture(textures[0], objectCoordinates);

    if (material.shader == 6 && material.textureCount > 1) {
        const vec4 layer = texture(textures[1], objectCoordinates);
        albedo = mix(albedo, layer, layer.a);
    }

    if (material.blendMode == 1 && albedo.a < 0.5) {
        discard;
    }

    if (material.blendMode == 0 || material.shader == 4) {
        albedo.a = 1.0;
    }

    const float diffuse = max(dot(normalize(worldNormal), -light.directionAmbient.xyz), 0.0);
    const float illumination = (material.flags & 1) != 0
        ? 1.0
        : light.directionAmbient.w + diffuse * light.colorIntensity.w;
    fragmentColor = vec4(albedo.rgb * light.colorIntensity.rgb * illumination, albedo.a);
}
