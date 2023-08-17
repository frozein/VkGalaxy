#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexPos;
layout(location = 2) in vec3 inNormal;

//----------------------------------------------------------------------------//

layout(location = 0) out vec2 texPos;
layout(location = 1) out vec3 normal;

//----------------------------------------------------------------------------//

layout(push_constant) uniform CameraConstants
{
	mat4 viewProj;
};

layout(binding = 0) buffer ModelBuffer
{
	mat4 models[];
};

//----------------------------------------------------------------------------//

void main() 
{
	texPos = inTexPos;
	normal = inNormal;

	gl_Position = vec4(inPos, 1.0);//vec4((viewProj * vec4(inPos, 1.0)).xyz, 1.0);
}