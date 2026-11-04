#version 450

layout(set = 1, binding = 0) uniform DirectionalLight {
    vec4 directionAmbient;
    vec4 colorIntensity;
} light;

layout(location = 0) in vec3 color;
layout(location = 1) in vec3 worldNormal;
layout(location = 0) out vec4 fragmentColor;

void main() {
    const float diffuse = max(dot(normalize(worldNormal), -light.directionAmbient.xyz), 0.0);
    const float illumination = light.directionAmbient.w + diffuse * light.colorIntensity.w;
    fragmentColor = vec4(color * light.colorIntensity.rgb * illumination, 1.0);
}
