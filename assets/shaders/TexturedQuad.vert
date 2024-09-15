#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

layout(binding = 0) uniform UniformBufferObject {
    mat4 MVP;
    float Phi;
} ubo;

void main() {
    float c = cos(ubo.Phi), s = sin(ubo.Phi);
    mat2 rot = mat2(c, -s, s, c);

    gl_Position = ubo.MVP * vec4(rot * inPosition, 0.0, 1.0);

    fragTexCoord = inTexCoord;
}