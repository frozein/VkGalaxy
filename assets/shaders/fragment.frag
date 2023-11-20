#version 450
#ifdef VULKAN

layout(location = 0) in vec2 texPos;

//----------------------------------------------------------------------------//

layout(location = 0) out vec4 outColor;

//----------------------------------------------------------------------------//

void main() 
{
	outColor = vec4(1.0);
}

#endif //#ifdef VULKAN