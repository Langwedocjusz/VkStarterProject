#version 450

layout(location = 0) in vec2 inPosition;

layout(location = 0) out int colorID;

layout(binding = 0) uniform UniformBufferObject {
    mat4 MVP;
    float PointSize;
    float Speed;
    float DeltaTime;
} ubo;

void main() {
    gl_PointSize = ubo.PointSize;
    gl_Position = ubo.MVP * vec4(inPosition, 0.0, 1.0);

    colorID = gl_VertexIndex;
}