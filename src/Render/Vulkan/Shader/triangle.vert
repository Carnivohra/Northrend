#version 450

layout(set = 0, binding = 0) uniform Camera {
    mat4 viewProjection;
} camera;

layout(push_constant) uniform Instance {
    mat4 transform;
} instance;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 vertexColor;

layout(location = 0) out vec3 color;

void main() {
    gl_Position =
        camera.viewProjection *
        instance.transform *
        vec4(position, 1.0);
    color = vertexColor;
}
