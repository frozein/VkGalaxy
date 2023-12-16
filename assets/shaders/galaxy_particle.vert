#version 430

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_texPos;

//----------------------------------------------------------------------------//

layout(location = 0) out vec2 o_texPos;

//----------------------------------------------------------------------------//

struct Particle
{
	vec3 pos;
	float scale;
};

//----------------------------------------------------------------------------//

layout(binding = 0) uniform Camera
{
	mat4 u_view;
	mat4 u_proj;
	mat4 u_viewProj;
};

layout(binding = 1) buffer PerInstance
{
	Particle u_particles[];
};

//----------------------------------------------------------------------------//

void main()
{
	vec3 camRight = vec3(u_view[0][0], u_view[1][0], u_view[2][0]);
	vec3 camUp = vec3(u_view[0][1], u_view[1][1], u_view[2][1]);

	Particle particle = u_particles[gl_InstanceIndex];
	vec3 worldspacePos = particle.pos + ((camRight * a_pos.x) + (camUp * a_pos.y)) * particle.scale;

	o_texPos = a_texPos;
	gl_Position = u_viewProj * vec4(worldspacePos, 1.0);
}