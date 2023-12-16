#version 450

layout(location = 0) in vec2 a_texPos;

//----------------------------------------------------------------------------//

layout(location = 0) out vec4 o_color;

//----------------------------------------------------------------------------//

layout(push_constant) uniform Params
{
	layout(offset = 64) vec2 u_offset;
	int u_numCells;
	float u_thickness;
	float u_scroll; // in [1, 2]
};

//----------------------------------------------------------------------------//

float ease_inout_exp(float x)
{
	return x <= 0.0 ? 0.0 : pow(2.0, 10.0 * (x - 1.0));
}

float ease_inout_quad(float x)
{
	if(x < 0.0)
		return 0.0;

	if(x < 0.5)
		return 2.0 * x * x;
	else
		return 1.0 - pow(-2.0 * x + 2.0, 2.0) / 2.0;
}

//----------------------------------------------------------------------------//

bool on_grid(vec2 pos, float thickness)
{
	thickness /= u_scroll;
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
		color += gridCol * ease_inout_quad(2.0 - 2.0 * u_scroll);
	if(on_grid(gridPos, u_thickness))
		color += gridCol * ease_inout_quad(2.0 * u_scroll - 1.0);

	color = min(color, gridCol);

	vec2 centeredPos = 2.0 * (a_texPos - 0.5 - u_offset) / u_scroll;
	color *= max(2.5 * ease_inout_exp(1.0 - length(centeredPos)), 0.0);

	o_color = vec4(color, 1.0);
}