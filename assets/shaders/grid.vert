#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexPos;

//----------------------------------------------------------------------------//

layout(location = 0) out vec2 texPos;

//----------------------------------------------------------------------------//

layout(binding = 0) buffer Camera
{
	mat4 viewProj;
};

layout(push_constant) uniform Model
{
	mat4 model;
};

//----------------------------------------------------------------------------//

void main() 
{
	texPos = inTexPos;
	gl_Position = viewProj * model * vec4(inPos, 1.0);
}