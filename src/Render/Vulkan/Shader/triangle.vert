#version 450

layout(set = 0, binding = 0) uniform Camera {
    mat4 viewProjection;
} camera;

layout(location = 0) out vec3 color;

const vec3 Positions[3] = vec3[](
    vec3(-1.0, -1.0, 0.0),
    vec3( 1.0, -1.0, 0.0),
    vec3( 0.0,  1.0, 0.0)
);

const vec3 Colors[3] = vec3[](
    vec3(0.20, 0.55, 1.00),
    vec3(0.20, 1.00, 0.65),
    vec3(0.90, 0.35, 1.00)
);

void main() {
    gl_Position = camera.viewProjection * vec4(Positions[gl_VertexIndex], 1.0);
    color = Colors[gl_VertexIndex];
}
