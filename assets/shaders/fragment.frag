#version 450

layout(location = 0) in vec2 texPos;

//----------------------------------------------------------------------------//

layout(location = 0) out vec4 outColor;

//----------------------------------------------------------------------------//

void main() 
{
	outColor = vec4(texPos, 0.5, 1.0);
}