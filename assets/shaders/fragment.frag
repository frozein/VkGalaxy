#ifdef VULKAN

#version 450

layout(location = 0) in vec2 texPos;
layout(location = 1) in vec3 normal;

//----------------------------------------------------------------------------//

layout(location = 0) out vec4 outColor;

//----------------------------------------------------------------------------//

layout(binding = 1) uniform sampler2D texAtlas;

//----------------------------------------------------------------------------//

void main() 
{
	outColor = vec4(1.0);//vec4(texture(texAtlas, texPos).xyz, 1.0);
}

#endif