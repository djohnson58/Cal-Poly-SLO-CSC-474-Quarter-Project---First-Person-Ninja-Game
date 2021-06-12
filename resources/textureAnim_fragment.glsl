#version 410 core
out vec4 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 vertex_tex;

uniform vec3 campos;

uniform vec2 texoff;
uniform vec2 texoff_last;
uniform float t;

uniform sampler2D tex;

void main()
{

vec2 start = vec2(vertex_tex.x/12, vertex_tex.y/7.0 - 0.01);
vec2 offset = vec2(texoff.x/12, texoff.y/7.0);
vec2 offset_last = vec2(texoff_last.x/12, texoff_last.y/7.0);
vec4 tcol = texture(tex, start + offset);
vec4 tcol2 = texture(tex, start + offset_last);
color = (tcol2 * (1 - t)) + (tcol * t);


}
