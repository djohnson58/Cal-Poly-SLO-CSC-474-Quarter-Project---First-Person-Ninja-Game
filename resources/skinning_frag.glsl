#version 330 core

in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 vertex_tex;

uniform sampler2D tex;
uniform vec3 camPos;

out vec4 fragCol;

void main()
{
	vec3 lightpos = vec3(0,30,0);
	vec3 lightdir = normalize(lightpos - vertex_pos);
	vec3 camdir = normalize(camPos - vertex_pos);
	vec3 frag_norm = normalize(vertex_normal);

	float diffuse_fact = clamp(dot(lightdir,frag_norm),0,1);
	diffuse_fact = 0.15 + diffuse_fact * 0.85;

	vec3 half = normalize(camdir + lightdir);
	float spec_fact = clamp(dot(half,frag_norm),0,1);

	vec4 tcol = texture(tex, vertex_tex);
	fragCol = tcol;

	fragCol.rgb = tcol.rgb*diffuse_fact*1.35 + 0.1*vec3(1,1,1)*spec_fact;
	if (fragCol.a < 0.1)
		discard;

	//fragCol = vec4(1,0,0,1);
	//vec4 tcol = texture(tex, vertex_tex);
	//fragCol.rgb = tcol.rgb;
}


