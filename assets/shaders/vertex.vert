#version 450
#ifdef VULKAN

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexPos;

//----------------------------------------------------------------------------//

layout(location = 0) out vec2 texPos;

//----------------------------------------------------------------------------//

layout(push_constant) uniform CameraConstants
{
	mat4 test;
};

layout(binding = 0) buffer Camera
{
	mat4 viewProj;
};

//----------------------------------------------------------------------------//

void main() 
{
	texPos = inTexPos;

	gl_Position = vec4((viewProj * vec4(inPos, 1.0)).xyz, 1.0);
}

#endif //#ifdef VULKAN