#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

layout(binding = 0) uniform UniformBufferObject {
    mat4 MVP;
    float Phi;
} ubo;

void main() {
    float c = cos(ubo.Phi), s = sin(ubo.Phi);
    mat2 rot = mat2(c, -s, s, c);

    vec2 pos2 = rot * inPosition.xz;

    vec3 pos = vec3(pos2.x, inPosition.y, pos2.y);

    gl_Position = ubo.MVP * vec4(pos, 1.0);

    fragTexCoord = inTexCoord;
}