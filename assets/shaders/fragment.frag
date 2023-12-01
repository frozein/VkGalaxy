#version 450

layout(location = 0) in vec2 texPos;

//----------------------------------------------------------------------------//

layout(location = 0) out vec4 outColor;

//----------------------------------------------------------------------------//

layout(push_constant) uniform Params
{
	layout(offset = 64) int numCells;
	float thickness;
	float scale;
};

//----------------------------------------------------------------------------//

void main() 
{
	vec2 gridPos = mod(texPos, 1.0 / numCells);
	gridPos *= numCells;

	if(gridPos.y > thickness && gridPos.y < 1.0 - thickness &&
	   gridPos.x > thickness && gridPos.x < 1.0 - thickness)
		discard;

	outColor = vec4(vec3(0.5), 1.0);
}