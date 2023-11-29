#ifndef DRAW_H
#define DRAW_H

#include "libs/render.hpp"
#include "libs/quickmath.hpp"

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

	VkBuffer cameraBuffers[FRAMES_IN_FLIGHT];
	VkDeviceMemory cameraBuffersMemory[FRAMES_IN_FLIGHT];
	VkBuffer cameraStagingBuffer;
	VkDeviceMemory cameraStagingBufferMemory;

	//grid pipeline objects:
	VkDescriptorSetLayout gridPipelineDescriptorLayout;
	VkPipelineLayout gridPipelineLayout;
	VkPipeline gridPipeline;

	VkBuffer gridVertexBuffer;
	VkDeviceMemory gridVertexBufferMemory;
	VkBuffer gridIndexBuffer;
	VkDeviceMemory gridIndexBufferMemory;

	VkDescriptorPool gridDescriptorPool;
	VkDescriptorSet gridDescriptorSets[FRAMES_IN_FLIGHT];
};

//----------------------------------------------------------------------------//

struct TerrainVertex
{
	qm::vec3 pos;
	qm::vec2 texCoord;
};

struct TerrainMesh
{
	uint64 id;
	qm::vec3 pos;

	uint64 vertexCount;
	TerrainVertex* mesh;
};

//----------------------------------------------------------------------------//

struct Camera
{
	qm::vec3 pos;
	qm::vec3 center;
	qm::vec3 up;

	float scale;
	float nearPlane, farPlane;

	//for game use only:
	float dist;
	float angle;
};

//----------------------------------------------------------------------------//

bool draw_init(DrawState** state);
void draw_quit(DrawState* state);

void draw_render(DrawState* state, Camera* cam);

#endif