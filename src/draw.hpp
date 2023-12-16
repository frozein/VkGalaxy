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

	//quad vertex buffers:
	VkBuffer quadVertexBuffer;
	VkDeviceMemory quadVertexBufferMemory;
	VkBuffer quadIndexBuffer;
	VkDeviceMemory quadIndexBufferMemory;

	//grid pipeline objects:
	VkDescriptorSetLayout gridPipelineDescriptorLayout;
	VkPipelineLayout gridPipelineLayout;
	VkPipeline gridPipeline;

	VkDescriptorPool gridDescriptorPool;
	VkDescriptorSet gridDescriptorSets[FRAMES_IN_FLIGHT];

	//galaxy particle pipeline objects:
	uint64 numGparticles;

	VkDescriptorSetLayout gparticlePipelineDescriptorLayout;
	VkPipelineLayout gparticlePipelineLayout;
	VkPipeline gparticlePipeline;

	VkDescriptorPool gparticleDescriptorPool;
	VkDescriptorSet gparticleDescriptorSets[FRAMES_IN_FLIGHT];

	VkDeviceSize gparticleBufferSize;
	VkBuffer gparticleBuffers[FRAMES_IN_FLIGHT];
	VkDeviceMemory gparticleBuffersMemory[FRAMES_IN_FLIGHT];
	VkBuffer gparticleStagingBuffer;
	VkDeviceMemory gparticleStagingBufferMemory;
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

struct Camera
{
	qm::vec3 pos;
	qm::vec3 up;
	qm::vec3 center;
	qm::vec3 targetCenter;

	float dist;
	float targetDist;
	float maxDist;

	float angle;
	float targetAngle;

	float fov;
	float nearPlane;
};

struct DrawParams
{
	Camera* cam;

	uint64 numParticles;
	qm::mat4* particleTransforms;
};

//----------------------------------------------------------------------------//

bool draw_init(DrawState** state);
void draw_quit(DrawState* state);

void draw_render(DrawState* state, DrawParams params);

#endif