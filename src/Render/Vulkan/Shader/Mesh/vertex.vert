#version 450

layout(set = 0, binding = 0) uniform Camera {
    mat4 viewProjection;
} camera;

layout(location = 0) in vec3 position;
layout(location = 1) in mat4 transform;

layout(location = 0) out vec3 color;

void main() {
    gl_Position =
        camera.viewProjection *
        transform *
        vec4(position, 1.0);

    const vec3 low = vec3(0.08, 0.24, 0.10);
    const vec3 high = vec3(0.68, 0.70, 0.66);
    const float height = clamp((position.z + 100.0) / 1200.0, 0.0, 1.0);
    color = mix(low, high, height);
}
