#version 430

#define NUM_VERTICES 6
const vec3 VERTICES[NUM_VERTICES] = {
	vec3(-1.0, 0.0, -1.0),
	vec3( 1.0, 0.0, -1.0),
	vec3(-1.0, 0.0,  1.0),
	vec3( 1.0, 0.0, -1.0),
	vec3(-1.0, 0.0,  1.0), 
	vec3( 1.0, 0.0,  1.0)
};

//----------------------------------------------------------------------------//

layout(location = 0) out vec2 o_texPos;
layout(location = 1) out float o_opacity;

//----------------------------------------------------------------------------//

struct Particle
{
	vec2 pos;
	float height;
	float angle;
	float tiltAngle;
	float angleVel;
	float opacity;
};

//----------------------------------------------------------------------------//

layout(binding = 0) uniform Camera
{
	mat4 u_view;
	mat4 u_proj;
	mat4 u_viewProj;
};

layout(push_constant) uniform Params
{
	float u_time;
};

layout(std140, binding = 1) readonly buffer Particles
{
	Particle particles[];
};

//----------------------------------------------------------------------------//

vec2 calc_pos(Particle particle)
{
	float angle = particle.angle + particle.angleVel * u_time;
	
	float cosAngle = cos(angle);
	float sinAngle = sin(angle);
	float cosTilt = cos(particle.tiltAngle);
	float sinTilt = sin(particle.tiltAngle);

	vec2 pos = particle.pos;

	return vec2(pos.x * cosAngle * cosTilt - pos.y * sinAngle * sinTilt,
	            pos.x * cosAngle * sinTilt + pos.y * sinAngle * cosTilt);
}

//----------------------------------------------------------------------------//

void main()
{
	vec3 a_pos    = VERTICES[gl_VertexIndex % NUM_VERTICES] * 0.5;
	vec2 a_texPos = VERTICES[gl_VertexIndex % NUM_VERTICES].xz;

	vec3 camRight = vec3(u_view[0][0], u_view[1][0], u_view[2][0]);
	vec3 camUp = vec3(u_view[0][1], u_view[1][1], u_view[2][1]);

	Particle particle = particles[gl_VertexIndex / NUM_VERTICES];
	vec2 pos = calc_pos(particle);
	vec3 worldspacePos = vec3(pos.x, particle.height, pos.y) + ((camRight * a_pos.x) + (camUp * a_pos.z)) * 10.0;

	o_texPos = a_texPos;
	o_opacity = particle.opacity;
	gl_Position = u_viewProj * vec4(worldspacePos, 1.0);
}