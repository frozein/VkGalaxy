#version 450

layout(location = 0) in vec2 a_texPos;
layout(location = 1) in float a_opacity;

//----------------------------------------------------------------------------//

layout(location = 0) out vec4 o_color;

//----------------------------------------------------------------------------//

void main() 
{
	vec2 centeredPos = 2.0 * (a_texPos - 0.5);
	if(dot(centeredPos, centeredPos) > 1.0)
		discard;

	o_color = vec4(vec3(1.0), a_opacity);
}