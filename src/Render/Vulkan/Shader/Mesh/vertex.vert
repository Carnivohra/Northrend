#version 450

layout(set = 0, binding = 0) uniform Camera {
    mat4 viewProjection;
} camera;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in mat4 transform;

layout(location = 0) out vec3 color;
layout(location = 1) out vec3 worldNormal;

void main() {
    gl_Position =
        camera.viewProjection *
        transform *
        vec4(position, 1.0);

    color = vec3(0.08, 0.24, 0.10);
    worldNormal = normalize(mat3(transform) * normal);
}
