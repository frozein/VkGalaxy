#ifndef GAMEDRAW_H
#define GAMEDRAW_H

#include "render.hpp"

//----------------------------------------------------------------------------//

struct DrawState
{
	RenderInstance* instance;

	VkFormat depthFormat;
	VkImage finalDepthImage;
	VkImageView finalDepthView;
	VkDeviceMemory finalDepthMemory;

	VkRenderPass finalRenderPass;

	VkDescriptorSetLayout terrainPipelineDescriptorLayout;
	VkPipelineLayout terrainPipelineLayout;
	VkPipeline terrainPipeline;
};

//----------------------------------------------------------------------------//

bool gamedraw_init(DrawState** state);
void gamedraw_quit(DrawState* state);

void gamedraw_draw(DrawState* state);

#endif