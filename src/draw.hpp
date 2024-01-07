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

	//grid pipeline objects:
	VKHgraphicsPipeline* gridPipeline;
	VKHdescriptorSets* gridDescriptorSets;

	//particle pipeline objects:
	VKHgraphicsPipeline* particlePipeline;
	VKHdescriptorSets* particleDescriptorSets;

	VkDeviceSize particleBufferSize;
	VkBuffer particleBuffer;
	VkDeviceMemory particleBufferMemory;
};

//----------------------------------------------------------------------------//

struct Vertex
{
	qm::vec3 pos;
	qm::vec2 texCoord;
};

struct GalaxyParticle
{
	qm::vec2 pos;
	f32 height;
	f32 angle;
	f32 tiltAngle;
	f32 angleVel;
	f32 opacity;
	f32 temp;
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
};

//----------------------------------------------------------------------------//

bool draw_init(DrawState** state);
void draw_quit(DrawState* state);

void draw_render(DrawState* state, DrawParams* params, f32 dt);

#endif