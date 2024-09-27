#version 450

layout(location = 0) in flat int colorID;

layout(location = 0) out vec4 outColor;

const vec3 palette[6] = vec3[6](
    vec3(26, 188, 156),
	vec3(46, 204, 113),
	vec3(52, 152, 219),
	vec3(155, 89, 182),
	vec3(52, 73, 94),
	vec3(22, 160, 133)
);

void main() {
    float r = length(gl_PointCoord - vec2(0.5));
    float fac = 0.5 - r;

    vec3 col = palette[colorID % 6]/255.0;

    outColor = vec4(vec3(col), fac);
}