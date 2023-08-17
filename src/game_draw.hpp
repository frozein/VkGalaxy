#ifndef GAMEDRAW_H
#define GAMEDRAW_H

#include "render.hpp"
#include "quickmath.hpp"

//----------------------------------------------------------------------------//

#define FRAMES_IN_FLIGHT 2

struct DrawState
{
	RenderInstance* instance;

	//drawing objects:
	VkFormat depthFormat;
	VkImage finalDepthImage;
	VkImageView finalDepthView;
	VkDeviceMemory finalDepthMemory;

	VkRenderPass finalRenderPass;

	uint32 framebufferCount;
	VkFramebuffer* framebuffers;

	VkCommandPool commandPool;
	VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT];

	VkSemaphore imageAvailableSemaphores[FRAMES_IN_FLIGHT];
	VkSemaphore renderFinishedSemaphores[FRAMES_IN_FLIGHT];
	VkFence inFlightFences[FRAMES_IN_FLIGHT];

	//terrain pipeline objects:
	VkDescriptorSetLayout terrainPipelineDescriptorLayout;
	VkPipelineLayout terrainPipelineLayout;
	VkPipeline terrainPipeline;

	VkBuffer terrainVertexBuffer;
	VkDeviceMemory terrainVertexBufferMemory;

	VkBuffer terrainStorageBuffers[FRAMES_IN_FLIGHT];
	VkDeviceMemory terrainUniformBufferMemory[FRAMES_IN_FLIGHT];
	void* terrainUniformBuffersMapped[FRAMES_IN_FLIGHT];

	VkImage terrainTextureAtlas;
	VkImageView terrainTextureAtlasView;
	VkDeviceMemory terrainTextureAtlasMemory;
	VkSampler terrainTextureAtlasSampler;

	VkDescriptorPool terrainDescriptorPool;
	VkDescriptorSet terrainDescriptorSets[FRAMES_IN_FLIGHT];
};

//----------------------------------------------------------------------------//

bool gamedraw_init(DrawState** state);
void gamedraw_quit(DrawState* state);

void gamedraw_draw(DrawState* state);

#endif