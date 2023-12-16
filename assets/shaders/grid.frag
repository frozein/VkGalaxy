#version 450

layout(location = 0) in vec2 a_texPos;

//----------------------------------------------------------------------------//

layout(location = 0) out vec4 o_color;

//----------------------------------------------------------------------------//

layout(push_constant) uniform Params
{
	layout(offset = 64) int u_numCells;
	float u_thickness;
	float u_scroll; // in [1, 2]
};

//----------------------------------------------------------------------------//

bool on_grid(vec2 pos, float thickness)
{
	return pos.y < thickness || pos.y > 1.0 - thickness ||
	       pos.x < thickness || pos.x > 1.0 - thickness;
}

void main() 
{
	const vec3 gridCol = vec3(0.5);

	vec2 gridPos = mod(a_texPos - 0.5, 1.0 / u_numCells);
	gridPos *= u_numCells;

	float halfThickness = u_thickness * 0.5;
	vec2 halfGridPos = mod(a_texPos - 0.5, 1.0 / (u_numCells * 2));
	halfGridPos *= (u_numCells * 2);

	vec3 color = vec3(0.0);
	if(on_grid(halfGridPos, halfThickness))
		color += gridCol * (2.0 - 2.0 * u_scroll);
	if(on_grid(gridPos, u_thickness))
		color += gridCol * (2.0 * u_scroll - 1.0);

	color = min(color, gridCol);
	o_color = vec4(color, 1.0);
}