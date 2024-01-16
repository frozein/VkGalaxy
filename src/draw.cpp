#include "draw.hpp"
#ifdef __APPLE__
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include <stdio.h>
#include <string.h>

//----------------------------------------------------------------------------//

#define DRAW_NUM_PARTICLES 80128
#define DRAW_NUM_STARS 75000

#define DRAW_PARTICLE_WORK_GROUP_SIZE 256

//----------------------------------------------------------------------------//

// mirrors camera buffer on GPU
struct CameraGPU
{
	qm::mat4 view;
	qm::mat4 proj;
	qm::mat4 viewProj;
};

//parameters for grid rendering
struct GridParamsVertGPU
{
	qm::mat4 model;
};

//parameters for grid rendering
struct GridParamsFragGPU
{
	qm::vec2 offset;
	int32 numCells;
	f32 thickness;
	f32 scroll;
};

//parameters for particle vertex shader
struct ParticleParamsVertGPU
{
	f32 time;

	uint32 numStars;

	f32 starSize;
	f32 dustSize;
	f32 h2Size;

	f32 h2Dist;
};

struct ParticleGenParamsGPU
{
	uint32 numStars;

	f32 maxRad;
	f32 bulgeRad;

	f32 angleOffset;
	f32 eccentricity;

	f32 baseHeight;
	f32 height;

	f32 minTemp;
	f32 maxTemp;
	f32 dustBaseTemp;

	f32 minStarOpacity;
	f32 maxStarOpacity;

	f32 minDustOpacity;
	f32 maxDustOpacity;

	f32 speed;
};

//----------------------------------------------------------------------------//

static bool _draw_create_depth_buffer(DrawState* state);
static void _draw_destroy_depth_buffer(DrawState* state);

static bool _draw_create_final_render_pass(DrawState* state);
static void _draw_destroy_final_render_pass(DrawState* state);

static bool _draw_create_framebuffers(DrawState* state);
static void _draw_destroy_framebuffers(DrawState* state);

static bool _draw_create_command_buffers(DrawState* state);
static void _draw_destroy_command_buffers(DrawState* state);

static bool _draw_create_sync_objects(DrawState* state);
static void _draw_destroy_sync_objects(DrawState* state);

static bool _draw_create_camera_buffer(DrawState* state);
static void _draw_destroy_camera_buffer(DrawState* state);

//----------------------------------------------------------------------------//

static bool _draw_create_quad_vertex_buffer(DrawState* state);
static void _draw_destroy_quad_vertex_buffer(DrawState* state);

//----------------------------------------------------------------------------//

static bool _draw_create_grid_pipeline(DrawState* state);
static void _draw_destroy_grid_pipeline(DrawState* state);

static bool _draw_create_grid_descriptors(DrawState* state);
static void _draw_destroy_grid_descriptors(DrawState* state);

//----------------------------------------------------------------------------//

static bool _draw_create_particle_pipeline(DrawState* state);
static void _draw_destroy_particle_pipeline(DrawState* state);

static bool _draw_create_particle_buffer(DrawState* state);
static void _draw_destroy_particle_buffer(DrawState* state);

static bool _draw_create_particle_descriptors(DrawState* state);
static void _draw_destroy_particle_descriptors(DrawState* state);

//----------------------------------------------------------------------------//

static bool _draw_initialize_particles(DrawState* state);

//----------------------------------------------------------------------------//

static void _draw_record_render_pass_start_commands(DrawState* s, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx);

static void _draw_record_particle_commands(DrawState* s, DrawParams* params, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx);
static void _draw_record_grid_commands(DrawState* s, DrawParams* params, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx);

//----------------------------------------------------------------------------//

static void _draw_window_resized(DrawState* state);

//----------------------------------------------------------------------------//

static void _draw_message_log(const char *message, const char *file, int32 line);
#define MSG_LOG(m) _draw_message_log(m, __FILENAME__, __LINE__)

static void _draw_error_log(const char *message, const char *file, int32 line);
#define ERROR_LOG(m) _draw_error_log(m, __FILENAME__, __LINE__)

//----------------------------------------------------------------------------//

bool draw_init(DrawState** state)
{
	*state = (DrawState* )malloc(sizeof(DrawState));
	DrawState* s = *state;

	//create render state:
	//---------------
	if(!vkh_init(&s->instance, 1920, 1080, "VkGalaxy"))
	{
		ERROR_LOG("failed to initialize render instance");
		return false;
	}

	//initialize objects for drawing:
	//---------------
	if(!_draw_create_depth_buffer(s))
		return false;

	if(!_draw_create_final_render_pass(s))
		return false;

	if(!_draw_create_framebuffers(s))
		return false;

	if(!_draw_create_command_buffers(s))
		return false;

	if(!_draw_create_sync_objects(s))
		return false;

	if(!_draw_create_camera_buffer(s))
		return false;

	//initialize reusable vertex buffers:
	//---------------
	if(!_draw_create_quad_vertex_buffer(s))
		return false;

	//initialize grid drawing objects:
	//---------------
	if(!_draw_create_grid_pipeline(s))
		return false;

	if(!_draw_create_grid_descriptors(s))
		return false;

	//initialize particle drawing objects:
	//---------------
	if(!_draw_create_particle_pipeline(s))
		return false;

	if(!_draw_create_particle_buffer(s))
		return false;

	if(!_draw_create_particle_descriptors(s))
		return false;

	if(!_draw_initialize_particles(s))
		return false;

	return true;
}

void draw_quit(DrawState* s)
{
	vkDeviceWaitIdle(s->instance->device);

	_draw_destroy_particle_descriptors(s);
	_draw_destroy_particle_buffer(s);
	_draw_destroy_particle_pipeline(s);

	_draw_destroy_grid_descriptors(s);
	_draw_destroy_grid_pipeline(s);

	_draw_destroy_quad_vertex_buffer(s);

	_draw_destroy_camera_buffer(s);
	_draw_destroy_sync_objects(s);
	_draw_destroy_command_buffers(s);
	_draw_destroy_framebuffers(s);
	_draw_destroy_final_render_pass(s);
	_draw_destroy_depth_buffer(s);

	vkh_quit(s->instance);

	free(s);
}

//----------------------------------------------------------------------------//

void draw_render(DrawState* s, DrawParams* params, f32 dt)
{
	static uint32 frameIdx = 0;

	//wait for fences and get next swapchain image: (essentially just making sure last frame is done):
	//---------------
	vkWaitForFences(s->instance->device, 1, &s->inFlightFences[frameIdx], VK_TRUE, UINT64_MAX);

	uint32 imageIdx;
	VkResult imageAquireResult = vkAcquireNextImageKHR(s->instance->device, s->instance->swapchain, UINT64_MAX,
													   s->imageAvailableSemaphores[frameIdx], VK_NULL_HANDLE, &imageIdx);
	if(imageAquireResult == VK_ERROR_OUT_OF_DATE_KHR || imageAquireResult == VK_SUBOPTIMAL_KHR)
	{
		_draw_window_resized(s);
		return;
	}
	else if(imageAquireResult != VK_SUCCESS)
	{
		ERROR_LOG("failed to acquire swapchain image");
		return;
	}

	vkResetFences(s->instance->device, 1, &s->inFlightFences[frameIdx]);

	//update camera buffer:
	//---------------
	int32 windowW, windowH;
	glfwGetWindowSize(s->instance->window, &windowW, &windowH);

	qm::mat4 view, projection;
	view = qm::lookat(params->cam.pos, params->cam.target, params->cam.up);
	projection = qm::perspective(params->cam.fov, (f32)windowW / (f32)windowH, 0.1f, INFINITY);

	CameraGPU camBuffer;
	camBuffer.view = view;
	camBuffer.proj = projection;
	camBuffer.viewProj = projection * view;
	vkh_copy_with_staging_buf(s->instance, s->cameraStagingBuffer, s->cameraStagingBufferMemory,
									  s->cameraBuffers[frameIdx], sizeof(CameraGPU), 0, &camBuffer);

	//start command buffer:
	//---------------
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	vkResetCommandBuffer(s->commandBuffers[frameIdx], 0);
	vkBeginCommandBuffer(s->commandBuffers[frameIdx], &beginInfo);

	//record commands:
	//---------------
	_draw_record_render_pass_start_commands(s, s->commandBuffers[frameIdx], frameIdx, imageIdx);

	_draw_record_grid_commands(s, params, s->commandBuffers[frameIdx], frameIdx, imageIdx);
	_draw_record_particle_commands(s, params, s->commandBuffers[frameIdx], frameIdx, imageIdx);

	//end command buffer:
	//---------------
	vkCmdEndRenderPass(s->commandBuffers[frameIdx]);

	if(vkEndCommandBuffer(s->commandBuffers[frameIdx]) != VK_SUCCESS)
		ERROR_LOG("failed to end command buffer");

	//submit command buffer:
	//---------------
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &s->imageAvailableSemaphores[frameIdx];
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &s->commandBuffers[frameIdx];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &s->renderFinishedSemaphores[frameIdx];

	vkQueueSubmit(s->instance->graphicsQueue, 1, &submitInfo, s->inFlightFences[frameIdx]);

	//present to screen:
	//---------------
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &s->renderFinishedSemaphores[frameIdx];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &s->instance->swapchain;
	presentInfo.pImageIndices = &imageIdx;
	presentInfo.pResults = nullptr;

	VkResult presentResult = vkQueuePresentKHR(s->instance->presentQueue, &presentInfo);
	if(presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
		_draw_window_resized(s);
	else if(presentResult != VK_SUCCESS)
		ERROR_LOG("failed to present swapchain image");

	frameIdx = (frameIdx + 1) % FRAMES_IN_FLIGHT;
}

//----------------------------------------------------------------------------//

static bool _draw_create_depth_buffer(DrawState* s)
{
	const uint32 possibleDepthFormatCount = 3;
	const VkFormat possibleDepthFormats[3] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};

	bool depthFormatFound = false;
	for(int32 i = 0; i < possibleDepthFormatCount; i++)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(s->instance->physicalDevice, possibleDepthFormats[i], &properties);

		if(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			s->depthFormat = possibleDepthFormats[i];
			depthFormatFound = true;
			break;
		}
	}

	if(!depthFormatFound)
	{
		ERROR_LOG("failed to find a supported depth buffer format");
		return false;
	}

	s->finalDepthImage = vkh_create_image(s->instance, s->instance->swapchainExtent.width, s->instance->swapchainExtent.height, 1,
											 VK_SAMPLE_COUNT_1_BIT, s->depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
											 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &s->finalDepthMemory);
	s->finalDepthView = vkh_create_image_view(s->instance, s->finalDepthImage, s->depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	return true;
}

static void _draw_destroy_depth_buffer(DrawState* s)
{
	vkh_destroy_image_view(s->instance, s->finalDepthView);
	vkh_destroy_image(s->instance, s->finalDepthImage, s->finalDepthMemory);
}

static bool _draw_create_final_render_pass(DrawState* s)
{
	//create attachments:
	//---------------
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = s->instance->swapchainFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = s->depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//create attachment references:
	//---------------
	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference = {};
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//create subpass description:
	//---------------
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;
	subpass.pDepthStencilAttachment = &depthAttachmentReference;

	//create dependency:
	//---------------
	VkSubpassDependency attachmentDependency = {};
	attachmentDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	attachmentDependency.dstSubpass = 0;
	attachmentDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	attachmentDependency.srcAccessMask = 0;
	attachmentDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	attachmentDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency dependencies[1] = {attachmentDependency};

	//create render pass:
	//---------------
	const uint32 attachmentCount = 2;
	VkAttachmentDescription attachments[2] = {colorAttachment, depthAttachment};

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = attachmentCount;
	renderPassCreateInfo.pAttachments = attachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = dependencies;

	if(vkCreateRenderPass(s->instance->device, &renderPassCreateInfo, nullptr, &s->finalRenderPass) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create final render pass");
		return false;
	}

	return true;
}

static void _draw_destroy_final_render_pass(DrawState* s)
{
	vkDestroyRenderPass(s->instance->device, s->finalRenderPass, NULL);
}

static bool _draw_create_framebuffers(DrawState* s)
{
	s->framebufferCount = s->instance->swapchainImageCount;
	s->framebuffers = (VkFramebuffer*)malloc(s->framebufferCount * sizeof(VkFramebuffer));

	for(uint32 i = 0; i < s->framebufferCount; i++)
	{
		VkImageView attachments[2] = {s->instance->swapchainImageViews[i], s->finalDepthView};

		VkFramebufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = s->finalRenderPass;
		createInfo.attachmentCount = 2;
		createInfo.pAttachments = attachments;
		createInfo.width = s->instance->swapchainExtent.width;
		createInfo.height = s->instance->swapchainExtent.height;
		createInfo.layers = 1;

		if(vkCreateFramebuffer(s->instance->device, &createInfo, nullptr, &s->framebuffers[i]) != VK_SUCCESS)
		{
			ERROR_LOG("failed to create framebuffer");
			return false;
		}
	}

	return true;
}

static void _draw_destroy_framebuffers(DrawState* s)
{
	for(uint32 i = 0; i < s->framebufferCount; i++)
		vkDestroyFramebuffer(s->instance->device, s->framebuffers[i], NULL);

	free(s->framebuffers);
}

static bool _draw_create_command_buffers(DrawState* s)
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = s->instance->graphicsComputeFamilyIdx;

	if(vkCreateCommandPool(s->instance->device, &poolInfo, nullptr, &s->commandPool) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create command pool");
		return false;
	}

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = s->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = FRAMES_IN_FLIGHT;

	if(vkAllocateCommandBuffers(s->instance->device, &allocInfo, s->commandBuffers) != VK_SUCCESS)
	{
		ERROR_LOG("failed to allocate command buffers");
		return false;
	}

	return true;
}

static void _draw_destroy_command_buffers(DrawState* s)
{
	vkFreeCommandBuffers(s->instance->device, s->commandPool, FRAMES_IN_FLIGHT, s->commandBuffers);
	vkDestroyCommandPool(s->instance->device, s->commandPool, NULL);
}

static bool _draw_create_sync_objects(DrawState* s)
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for(int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
		if(vkCreateSemaphore(s->instance->device, &semaphoreInfo, NULL, &s->imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(s->instance->device, &semaphoreInfo, NULL, &s->renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(s->instance->device, &fenceInfo, NULL, &s->inFlightFences[i]) != VK_SUCCESS)
		{
			ERROR_LOG("failed to create sync objects");
			return false;
		}

	return true;
}

static void _draw_destroy_sync_objects(DrawState* s)
{
	for(int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(s->instance->device, s->imageAvailableSemaphores[i], NULL);
		vkDestroySemaphore(s->instance->device, s->renderFinishedSemaphores[i], NULL);
		vkDestroyFence(s->instance->device, s->inFlightFences[i], NULL);
	}
}

static bool _draw_create_camera_buffer(DrawState* s)
{
	VkDeviceSize bufferSize = sizeof(CameraGPU);

	for(int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		s->cameraBuffers[i] = vkh_create_buffer(s->instance, bufferSize,
												   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
												   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &s->cameraBuffersMemory[i]);
	}

	s->cameraStagingBuffer = vkh_create_buffer(s->instance, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
												  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &s->cameraStagingBufferMemory);

	return true;
}

static void _draw_destroy_camera_buffer(DrawState* s)
{
	vkh_destroy_buffer(s->instance, s->cameraStagingBuffer, s->cameraStagingBufferMemory);

	for(int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
		vkh_destroy_buffer(s->instance, s->cameraBuffers[i], s->cameraBuffersMemory[i]);
}

//----------------------------------------------------------------------------//

static bool _draw_create_quad_vertex_buffer(DrawState* s)
{
	Vertex verts[4] = {{{-0.5f, 0.0f, -0.5f}, {0.0f, 0.0f}}, {{0.5f, 0.0f, -0.5f}, {1.0f, 0.0f}}, {{-0.5f, 0.0f, 0.5f}, {0.0f, 1.0f}}, {{0.5f, 0.0f, 0.5f}, {1.0f, 1.0f}}};
	uint32 indices[6] = {0, 1, 2, 1, 2, 3};

	s->quadVertexBuffer = vkh_create_buffer(s->instance, sizeof(verts),
												  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
												  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &s->quadVertexBufferMemory);
	vkh_copy_with_staging_buf_implicit(s->instance, s->quadVertexBuffer, sizeof(verts), 0, verts);

	s->quadIndexBuffer = vkh_create_buffer(s->instance, sizeof(indices),
												 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
												 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &s->quadIndexBufferMemory);
	vkh_copy_with_staging_buf_implicit(s->instance, s->quadIndexBuffer, sizeof(indices), 0, indices);

	return true;
}

static void _draw_destroy_quad_vertex_buffer(DrawState* s)
{
	vkh_destroy_buffer(s->instance, s->quadVertexBuffer, s->quadVertexBufferMemory);
	vkh_destroy_buffer(s->instance, s->quadIndexBuffer, s->quadIndexBufferMemory);
}

//----------------------------------------------------------------------------//

static bool _draw_create_grid_pipeline(DrawState* s)
{
	//create pipeline object:
	//---------------
	s->gridPipeline = vkh_pipeline_create();
	if(!s->gridPipeline)
		return false;

	//set shaders:
	//---------------
	uint64 vertCodeSize, fragCodeSize;
	uint32 *vertCode = vkh_load_spirv("assets/spirv/grid.vert.spv", &vertCodeSize);
	uint32 *fragCode = vkh_load_spirv("assets/spirv/grid.frag.spv", &fragCodeSize);

	VkShaderModule vertModule = vkh_create_shader_module(s->instance, vertCodeSize, vertCode);
	VkShaderModule fragModule = vkh_create_shader_module(s->instance, fragCodeSize, fragCode);

	vkh_pipeline_set_vert_shader(s->gridPipeline, vertModule);
	vkh_pipeline_set_frag_shader(s->gridPipeline, fragModule);

	//add descriptor set layout bindings:
	//---------------
	VkDescriptorSetLayoutBinding storageLayoutBinding = {};
	storageLayoutBinding.binding = 0;
	storageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	storageLayoutBinding.descriptorCount = 1; //camera
	storageLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	storageLayoutBinding.pImmutableSamplers = nullptr;

	vkh_pipeline_add_desc_set_binding(s->gridPipeline, storageLayoutBinding);

	//add dynamic states:
	//---------------
	vkh_pipeline_add_dynamic_state(s->gridPipeline, VK_DYNAMIC_STATE_VIEWPORT);
	vkh_pipeline_add_dynamic_state(s->gridPipeline, VK_DYNAMIC_STATE_SCISSOR);

	//add vertex input info:
	//---------------
	VkVertexInputBindingDescription vertBindingDescription = {};
	vertBindingDescription.binding = 0;
	vertBindingDescription.stride = sizeof(Vertex);
	vertBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	vkh_pipeline_add_vertex_input_binding(s->gridPipeline, vertBindingDescription);

	VkVertexInputAttributeDescription vertPositionAttrib = {};
	vertPositionAttrib.binding = 0;
	vertPositionAttrib.location = 0;
	vertPositionAttrib.format = VK_FORMAT_R32G32B32_SFLOAT;
	vertPositionAttrib.offset = offsetof(Vertex, pos);

	VkVertexInputAttributeDescription vertTexCoordAttrib = {};
	vertTexCoordAttrib.binding = 0;
	vertTexCoordAttrib.location = 1;
	vertTexCoordAttrib.format = VK_FORMAT_R32G32_SFLOAT;
	vertTexCoordAttrib.offset = offsetof(Vertex, texCoord);

	vkh_pipeline_add_vertex_input_attrib(s->gridPipeline, vertPositionAttrib);
	vkh_pipeline_add_vertex_input_attrib(s->gridPipeline, vertTexCoordAttrib);

	//add color blend attachments:
	//---------------
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	vkh_pipeline_add_color_blend_attachment(s->gridPipeline, colorBlendAttachment);

	//add push constsants:
	//---------------
	VkPushConstantRange vertPushConstant = {};
	vertPushConstant.offset = 0;
	vertPushConstant.size = sizeof(GridParamsVertGPU);
	vertPushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkPushConstantRange fragPushConstant = {};
	fragPushConstant.offset = sizeof(GridParamsVertGPU);
	fragPushConstant.size = sizeof(GridParamsFragGPU);
	fragPushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	vkh_pipeline_add_push_constant(s->gridPipeline, vertPushConstant);
	vkh_pipeline_add_push_constant(s->gridPipeline, fragPushConstant);

	//set states:
	//---------------
	vkh_pipeline_set_input_assembly_state(s->gridPipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

	vkh_pipeline_set_raster_state(s->gridPipeline, VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
		VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, 0.0f, 0.0f, 0.0f);

	vkh_pipeline_set_multisample_state(s->gridPipeline, VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 1.0f, NULL, VK_FALSE, VK_FALSE);

	vkh_pipeline_set_depth_stencil_state(s->gridPipeline, VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, VK_FALSE, {}, {}, 0.0f, 1.0f);

	vkh_pipeline_set_color_blend_state(s->gridPipeline, VK_FALSE, VK_LOGIC_OP_COPY, 0.0f, 0.0f, 0.0f, 0.0f);

	//generate pipeline:
	//---------------
	if(!vkh_pipeline_generate(s->gridPipeline, s->instance, s->finalRenderPass, 0))
		return false;

	//cleanup:
	//---------------
	vkh_free_spirv(vertCode);
	vkh_free_spirv(fragCode);

	vkh_destroy_shader_module(s->instance, vertModule);
	vkh_destroy_shader_module(s->instance, fragModule);

	return true;
}

static void _draw_destroy_grid_pipeline(DrawState* s)
{
	vkh_pipeline_cleanup(s->gridPipeline, s->instance);
	vkh_pipeline_destroy(s->gridPipeline);
}

static bool _draw_create_grid_descriptors(DrawState* s)
{
	s->gridDescriptorSets = vkh_descriptor_sets_create(FRAMES_IN_FLIGHT);
	if(!s->gridDescriptorSets)
		return false;

	VkDescriptorBufferInfo cameraBufferInfos[FRAMES_IN_FLIGHT];
	for(int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		cameraBufferInfos[i].buffer = s->cameraBuffers[i];
		cameraBufferInfos[i].offset = 0;
		cameraBufferInfos[i].range = sizeof(CameraGPU);

		vkh_descriptor_sets_add_buffers(s->gridDescriptorSets, i, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
			0, 0, 1, &cameraBufferInfos[i]);
	}

	return vkh_desctiptor_sets_generate(s->gridDescriptorSets, s->instance, s->gridPipeline->descriptorLayout);
}

static void _draw_destroy_grid_descriptors(DrawState* s)
{
	vkh_descriptor_sets_cleanup(s->gridDescriptorSets, s->instance);
	vkh_descriptor_sets_destroy(s->gridDescriptorSets);
}

//----------------------------------------------------------------------------//

static bool _draw_create_particle_pipeline(DrawState* s)
{
	//create pipeline object:
	//---------------
	s->particlePipeline = vkh_pipeline_create();
	if(!s->particlePipeline)
		return false;

	//set shaders:
	//---------------
	uint64 vertCodeSize, fragCodeSize;
	uint32 *vertCode = vkh_load_spirv("assets/spirv/particle.vert.spv", &vertCodeSize);
	uint32 *fragCode = vkh_load_spirv("assets/spirv/particle.frag.spv", &fragCodeSize);

	VkShaderModule vertModule = vkh_create_shader_module(s->instance, vertCodeSize, vertCode);
	VkShaderModule fragModule = vkh_create_shader_module(s->instance, fragCodeSize, fragCode);

	vkh_pipeline_set_vert_shader(s->particlePipeline, vertModule);
	vkh_pipeline_set_frag_shader(s->particlePipeline, fragModule);

	//add descriptor set layout bindings:
	//---------------
	VkDescriptorSetLayoutBinding cameraLayoutBinding = {};
	cameraLayoutBinding.binding = 0;
	cameraLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	cameraLayoutBinding.descriptorCount = 1;
	cameraLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	cameraLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding particleLayoutBinding = {};
	particleLayoutBinding.binding = 1;
	particleLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	particleLayoutBinding.descriptorCount = 1;
	particleLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	particleLayoutBinding.pImmutableSamplers = nullptr;

	vkh_pipeline_add_desc_set_binding(s->particlePipeline, cameraLayoutBinding);
	vkh_pipeline_add_desc_set_binding(s->particlePipeline, particleLayoutBinding);

	//add dynamic states:
	//---------------
	vkh_pipeline_add_dynamic_state(s->particlePipeline, VK_DYNAMIC_STATE_VIEWPORT);
	vkh_pipeline_add_dynamic_state(s->particlePipeline, VK_DYNAMIC_STATE_SCISSOR);

	//add color blend attachments:
	//---------------
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;

	vkh_pipeline_add_color_blend_attachment(s->particlePipeline, colorBlendAttachment);

	//add push constsants:
	//---------------
	VkPushConstantRange vertPushConstant = {};
	vertPushConstant.offset = 0;
	vertPushConstant.size = sizeof(ParticleParamsVertGPU);
	vertPushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	vkh_pipeline_add_push_constant(s->particlePipeline, vertPushConstant);

	//set states:
	//---------------
	vkh_pipeline_set_input_assembly_state(s->particlePipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

	vkh_pipeline_set_raster_state(s->particlePipeline, VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
		VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, 0.0f, 0.0f, 0.0f);

	vkh_pipeline_set_multisample_state(s->particlePipeline, VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 1.0f, NULL, VK_FALSE, VK_FALSE);

	vkh_pipeline_set_depth_stencil_state(s->particlePipeline, VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS, VK_FALSE, VK_FALSE, {}, {}, 0.0f, 1.0f);

	vkh_pipeline_set_color_blend_state(s->particlePipeline, VK_FALSE, VK_LOGIC_OP_COPY, 0.0f, 0.0f, 0.0f, 0.0f);

	//generate pipeline:
	//---------------
	if(!vkh_pipeline_generate(s->particlePipeline, s->instance, s->finalRenderPass, 0))
		return false;

	//cleanup:
	//---------------
	vkh_free_spirv(vertCode);
	vkh_free_spirv(fragCode);

	vkh_destroy_shader_module(s->instance, vertModule);
	vkh_destroy_shader_module(s->instance, fragModule);

	return true;
}

static void _draw_destroy_particle_pipeline(DrawState* s)
{
	vkh_pipeline_cleanup(s->particlePipeline, s->instance);
	vkh_pipeline_destroy(s->particlePipeline);
}

static bool _draw_create_particle_buffer(DrawState* s)
{
	s->particleBufferSize = DRAW_NUM_PARTICLES * sizeof(GalaxyParticle);

	s->particleBuffer = vkh_create_buffer(s->instance, s->particleBufferSize,
												VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
												VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &s->particleBufferMemory);

	return true;
}

static void _draw_destroy_particle_buffer(DrawState* s)
{
	vkh_destroy_buffer(s->instance, s->particleBuffer, s->particleBufferMemory);
}

static bool _draw_create_particle_descriptors(DrawState* s)
{
	s->particleDescriptorSets = vkh_descriptor_sets_create(FRAMES_IN_FLIGHT);
	if(!s->particleDescriptorSets)
		return false;

	VkDescriptorBufferInfo cameraBufferInfos[FRAMES_IN_FLIGHT];
	VkDescriptorBufferInfo particleBufferInfos[FRAMES_IN_FLIGHT];
	for(int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		cameraBufferInfos[i].buffer = s->cameraBuffers[i];
		cameraBufferInfos[i].offset = 0;
		cameraBufferInfos[i].range = sizeof(CameraGPU);

		particleBufferInfos[i].buffer = s->particleBuffer;
		particleBufferInfos[i].offset = 0;
		particleBufferInfos[i].range = VK_WHOLE_SIZE;

		vkh_descriptor_sets_add_buffers(s->particleDescriptorSets, i, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
			0, 0, 1, &cameraBufferInfos[i]);

		vkh_descriptor_sets_add_buffers(s->particleDescriptorSets, i, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 
			1, 0, 1, &particleBufferInfos[i]);
	}

	return vkh_desctiptor_sets_generate(s->particleDescriptorSets, s->instance, s->particlePipeline->descriptorLayout);
}

static void _draw_destroy_particle_descriptors(DrawState* s)
{
	vkh_descriptor_sets_cleanup(s->particleDescriptorSets, s->instance);
	vkh_descriptor_sets_destroy(s->particleDescriptorSets);
}

//----------------------------------------------------------------------------//

static bool _draw_initialize_particles(DrawState* s)
{
	VKHcomputePipeline* pipeline;
	VKHdescriptorSets* descriptorSets;

	//create pipeline:
	//---------------
	pipeline = vkh_compute_pipeline_create();
	if(!pipeline)
		return false;

	uint64 computeCodeSize;
	uint32 *computeCode = vkh_load_spirv("assets/spirv/particle_generate.comp.spv", &computeCodeSize);
	VkShaderModule computeModule = vkh_create_shader_module(s->instance, computeCodeSize, computeCode);
	vkh_compute_pipeline_set_shader(pipeline, computeModule);

	VkDescriptorSetLayoutBinding particleLayoutBinding = {};
	particleLayoutBinding.binding = 0;
	particleLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	particleLayoutBinding.descriptorCount = 1;
	particleLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	particleLayoutBinding.pImmutableSamplers = nullptr;

	vkh_compute_pipeline_add_desc_set_binding(pipeline, particleLayoutBinding);

	VkPushConstantRange pushConstant = {};
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pushConstant.offset = 0;
	pushConstant.size = sizeof(ParticleGenParamsGPU);

	vkh_compute_pipeline_add_push_constant(pipeline, pushConstant);

	if(!vkh_compute_pipeline_generate(pipeline, s->instance))
		return false;

	//create descriptor sets:
	//---------------
	descriptorSets = vkh_descriptor_sets_create(1);
	if(!descriptorSets)
		return false;

	VkDescriptorBufferInfo particleBufferInfo = {};
	particleBufferInfo.buffer = s->particleBuffer;
	particleBufferInfo.offset = 0;
	particleBufferInfo.range = VK_WHOLE_SIZE;

	vkh_descriptor_sets_add_buffers(descriptorSets, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
		0, 0, 1, &particleBufferInfo);
	
	if(!vkh_desctiptor_sets_generate(descriptorSets, s->instance, pipeline->descriptorLayout))
		return false;

	//run pipeline:
	//---------------
	VkCommandBuffer commandBuf = vkh_start_single_time_command(s->instance);

	ParticleGenParamsGPU params;
	params.numStars = DRAW_NUM_STARS;
	params.maxRad = 3500.0f;
	params.bulgeRad = 1250.0f;
	params.angleOffset = 6.28f;
	params.eccentricity = 0.85f;
	params.baseHeight = 300.0f;
	params.height = 250.0f;
	params.minTemp = 3000.0f;
	params.maxTemp = 9000.0f;
	params.dustBaseTemp = 4000.0f;
	params.minStarOpacity = 0.1f;
	params.maxStarOpacity = 0.5f;
	params.minDustOpacity = 0.01f;
	params.maxDustOpacity = 0.05f;
	params.speed = 10.0f;

	uint32 dynamicOffset = 0;
	vkCmdBindPipeline(commandBuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipeline);
	vkCmdBindDescriptorSets(commandBuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->layout, 0, 1, &descriptorSets->sets[0], 1, &dynamicOffset);
	vkCmdPushConstants(commandBuf, pipeline->layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ParticleGenParamsGPU), &params);
	vkCmdDispatch(commandBuf, DRAW_NUM_PARTICLES / DRAW_PARTICLE_WORK_GROUP_SIZE, 1, 1);

	vkh_end_single_time_command(s->instance, commandBuf);

	vkDeviceWaitIdle(s->instance->device);

	//cleanup:
	//---------------
	vkh_descriptor_sets_cleanup(descriptorSets, s->instance);
	vkh_descriptor_sets_destroy(descriptorSets);
	
	vkh_compute_pipeline_cleanup(pipeline, s->instance);
	vkh_compute_pipeline_destroy(pipeline);

	vkh_free_spirv(computeCode);
	vkh_destroy_shader_module(s->instance, computeModule);

	return true;
}

//----------------------------------------------------------------------------//

static void _draw_record_render_pass_start_commands(DrawState* s, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx)
{
	//render pass begin:
	//---------------
	VkRenderPassBeginInfo renderBeginInfo = {};
	renderBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderBeginInfo.renderPass = s->finalRenderPass;
	renderBeginInfo.framebuffer = s->framebuffers[imageIdx];
	renderBeginInfo.renderArea.offset = {0, 0};
	renderBeginInfo.renderArea.extent = s->instance->swapchainExtent;

	VkClearValue clearValues[3];
	clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clearValues[1].depthStencil = {1.0f, 0};
	clearValues[2].color = {{0.0f, 0.0f, 0.0f, 1.0f}};

	renderBeginInfo.clearValueCount = 3;
	renderBeginInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(commandBuffer, &renderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	//set viewport and scissor:
	//---------------
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = (f32)s->instance->swapchainExtent.height;
	viewport.width = (f32)s->instance->swapchainExtent.width;
	viewport.height = -(f32)s->instance->swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = s->instance->swapchainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

static void _draw_record_grid_commands(DrawState* s, DrawParams* params, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s->gridPipeline->pipeline);

	//bind buffers:
	//---------------
	VkBuffer vertexBuffers[] = {s->quadVertexBuffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, s->quadIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s->gridPipeline->layout, 0, 1, &s->gridDescriptorSets->sets[frameIndex], 0, nullptr);

	int32 numCells = 16;

	//send vertex stage params:
	//---------------
	int32 windowW, windowH;
	glfwGetWindowSize(s->instance->window, &windowW, &windowH);	//TODO: FIGURE OUT WHY IT GETS CUT OFF WITH VERY TALL WINDOWS
	f32 aspect = (f32)windowW / (f32)windowH;
	if(aspect < 1.0f)
		aspect = 1.0f / aspect;

	f32 size = aspect * powf(2.0f, roundf(log2f(params->cam.dist) + 0.5f));

	qm::vec3 pos = params->cam.target;
	for(int32 i = 0; i < 3; i++)
		pos[i] -= fmodf(pos[i], size / numCells);

	qm::mat4 model = qm::translate(pos) * qm::scale(qm::vec3(size, size, size));

	GridParamsVertGPU vertParams = {model};
	vkCmdPushConstants(commandBuffer, s->gridPipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GridParamsVertGPU), &vertParams);

	//send fragment stage params:
	//---------------
	f32 thickness = 0.0125f;
	f32 scroll = (params->cam.dist - powf(2.0f, roundf(log2f(params->cam.dist) - 0.5f))) / (4.0f * powf(2.0f, roundf(log2f(params->cam.dist) - 1.5f))) + 0.5f;
	qm::vec3 offset3 = (params->cam.target - pos) / size;
	qm::vec2 offset = qm::vec2(offset3.x, offset3.z);

	GridParamsFragGPU fragParams = {offset, numCells, thickness, scroll};
	vkCmdPushConstants(commandBuffer, s->gridPipeline->layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(GridParamsVertGPU), sizeof(GridParamsFragGPU), &fragParams);

	//draw:
	//---------------
	vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
}

static void _draw_record_particle_commands(DrawState* s, DrawParams* params, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s->particlePipeline->pipeline);

	//bind descriptor sets:
	//---------------
	uint32 dynamicOffset = 0;
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s->particlePipeline->layout, 0, 1, &s->particleDescriptorSets->sets[frameIndex], 1, &dynamicOffset);

	//send vertex stage params:
	//---------------
	ParticleParamsVertGPU vertParams;
	vertParams.time = (float)glfwGetTime();
	vertParams.numStars = DRAW_NUM_STARS;
	vertParams.starSize = 10.0f;
	vertParams.dustSize = 500.0f;
	vertParams.h2Size = 150.0f;
	vertParams.h2Dist = 300.0f;

	vkCmdPushConstants(commandBuffer, s->particlePipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ParticleParamsVertGPU), &vertParams);

	//draw:
	//---------------
	vkCmdDraw(commandBuffer, 6 * DRAW_NUM_PARTICLES, 1, 0, 0);
}

//----------------------------------------------------------------------------//

static void _draw_window_resized(DrawState* s)
{
	int32 w, h;
	glfwGetFramebufferSize(s->instance->window, &w, &h);
	if(w == 0 || h == 0)
		return;

	vkh_resize_swapchain(s->instance, w, h);

	_draw_destroy_depth_buffer(s);
	_draw_create_depth_buffer(s);

	_draw_destroy_framebuffers(s);
	_draw_create_framebuffers(s);
}

//----------------------------------------------------------------------------//

static void _draw_message_log(const char* message, const char* file, int32 line)
{
	printf("DRAW MESSAGE in %s at line %i - \"%s\"\n\n", file, line, message);
}

static void _draw_error_log(const char* message, const char* file, int32 line)
{
	printf("DRAW ERROR in %s at line %i - \"%s\"\n\n", file, line, message);
}