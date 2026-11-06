#version 450

layout(set = 0, binding = 0) uniform Camera {
    mat4 viewProjection;
} camera;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 textureCoordinates;
layout(location = 3) in uvec4 textureIndices;
layout(location = 4) in uint materialData;

layout(location = 0) out vec3 worldNormal;
layout(location = 1) out vec2 terrainCoordinates;
layout(location = 2) flat out uvec4 terrainTextures;
layout(location = 3) flat out uint terrainMaterial;

void main() {
    gl_Position = camera.viewProjection * vec4(position, 1.0);

    worldNormal = normal;
    terrainCoordinates = textureCoordinates;
    terrainTextures = textureIndices;
    terrainMaterial = materialData;
}
