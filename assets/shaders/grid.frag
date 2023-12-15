#version 450

layout(location = 0) in vec2 texPos;

//----------------------------------------------------------------------------//

layout(location = 0) out vec4 outColor;

//----------------------------------------------------------------------------//

layout(push_constant) uniform Params
{
	layout(offset = 64) int numCells;
	float thickness;
	float scroll;
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

	vec2 gridPos = mod(texPos - 0.5, 1.0 / numCells);
	gridPos *= numCells;

	float halfThickness = thickness * 0.5;
	vec2 halfGridPos = mod(texPos - 0.5, 1.0 / (numCells * 2));
	halfGridPos *= (numCells * 2);

	vec3 color = vec3(0.0);
	if(on_grid(halfGridPos, halfThickness))
		color += gridCol * (2.0 - 2.0 * scroll);
	if(on_grid(gridPos, thickness))
		color += gridCol * (2.0 * scroll - 1.0);

	color = min(color, gridCol);
	outColor = vec4(color, 1.0);
}