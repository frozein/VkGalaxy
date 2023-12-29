#ifndef DRAW_H
#define DRAW_H

#include "libs/vkh/vkh.h"
#include "libs/quickmath.hpp"

#include "globals.hpp"

//----------------------------------------------------------------------------//

#define FRAMES_IN_FLIGHT 2

struct DrawState
{
	VKHinstance* instance;

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

	VkBuffer cameraBuffers[FRAMES_IN_FLIGHT];
	VkDeviceMemory cameraBuffersMemory[FRAMES_IN_FLIGHT];
	VkBuffer cameraStagingBuffer;
	VkDeviceMemory cameraStagingBufferMemory;

	//quad vertex buffers:
	VkBuffer quadVertexBuffer;
	VkDeviceMemory quadVertexBufferMemory;
	VkBuffer quadIndexBuffer;
	VkDeviceMemory quadIndexBufferMemory;

	//grid pipeline object
	VKHgraphicsPipeline* gridPipeline;
	VKHdescriptorSets* gridDescriptorSets;

	//galaxy particle pipeline objects:
	uint64 numGparticles;

	VKHgraphicsPipeline* particlePipeline;
	VKHdescriptorSets* particleDescriptorSets;

	VkDescriptorSetLayout gparticleComputePipelineDescriptorLayout;
	VkPipelineLayout gparticleComputePipelineLayout;
	VkPipeline gparticleComputePipeline;
	VKHdescriptorSets* particleComputeDescriptorSets;

	VkDeviceSize gparticleBufferSize;
	VkBuffer gparticleBuffer;
	VkDeviceMemory gparticleBufferMemory;
};

//----------------------------------------------------------------------------//

struct Vertex
{
	qm::vec3 pos;
	qm::vec2 texCoord;
};

struct GalaxyParticle
{
	qm::vec3 pos;
	float scale;
};

//----------------------------------------------------------------------------//

struct DrawParams
{
	struct
	{
		qm::vec3 pos;
		qm::vec3 up;
		qm::vec3 target;

		float dist;

		float fov;
	} cam;

	uint32 numParticles;
	qm::mat4* particleTransforms;
};

//----------------------------------------------------------------------------//

bool draw_init(DrawState** state);
void draw_quit(DrawState* state);

void draw_render(DrawState* state, DrawParams* params);

#endif