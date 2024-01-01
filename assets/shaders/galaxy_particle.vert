#version 430

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_texPos;

//----------------------------------------------------------------------------//

layout(location = 0) out vec2 o_texPos;
layout(location = 1) flat out uint o_instanceIdx;

//----------------------------------------------------------------------------//

struct Particle
{
	vec2 pos;
	float height;
	float angle;
	float tiltAngle;
	float angleVel;
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
	vec3 camRight = vec3(u_view[0][0], u_view[1][0], u_view[2][0]);
	vec3 camUp = vec3(u_view[0][1], u_view[1][1], u_view[2][1]);

	Particle particle = particles[gl_InstanceIndex];
	vec2 pos = calc_pos(particle);
	vec3 worldspacePos = vec3(pos.x, particle.height, pos.y) + ((camRight * a_pos.x) + (camUp * a_pos.z)) * 25.0;

	o_texPos = a_texPos;
	o_instanceIdx = gl_InstanceIndex;
	gl_Position = u_viewProj * vec4(worldspacePos, 1.0);
}