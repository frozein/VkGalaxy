#version 450

layout(location = 0) in vec2 a_texPos;
layout(location = 1) in vec4 a_color;
layout(location = 2) in flat uint a_type;

//----------------------------------------------------------------------------//

layout(location = 0) out vec4 o_color;

//----------------------------------------------------------------------------//

void main() 
{
	vec2 centeredPos = 2.0 * (a_texPos - 0.5);
	
	vec4 color = a_color;
	if(a_type == 0) //stars
	{
		if(dot(centeredPos, centeredPos) > 1.0)
			discard;
	}
	else if(a_type == 1) //dust
	{
		color.rgb *= vec3(0.5, 0.5, 1.0);
		color.a *= max(1.0 - length(centeredPos), 0.0);
	}
	else //h-2 regions
	{
		float dist = max(1.0 - length(centeredPos), 0.0);

		color.rgb = mix(vec3(1.0, 0.0, 0.0), vec3(1.0), dist * dist * dist);
		color.a = dist * dist;
	}

	o_color = color;
}