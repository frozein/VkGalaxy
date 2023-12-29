#include "draw.hpp"
#include <malloc.h>
#include <stdio.h>
#include <string.h>

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

//parameters for particle compute shader
struct ParticleComputeParamsGPU
{
	f32 time;
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

static bool _draw_create_gparticle_graphics_pipeline(DrawState* state);
static void _draw_destroy_gparticle_graphics_pipeline(DrawState* state);

static bool _draw_create_gparticle_compute_pipeline(DrawState* state);
static void _draw_destroy_gparticle_compute_pipeline(DrawState* state);

static bool _draw_create_gparticle_buffer(DrawState* state);
static void _draw_destroy_gparticle_buffer(DrawState* state);

static bool _draw_create_gparticle_graphics_descriptors(DrawState* state);
static void _draw_destroy_gparticle_graphics_descriptors(DrawState* state);

static bool _draw_create_gparticle_compute_descriptors(DrawState* state);
static void _draw_destroy_gparticle_compute_descriptors(DrawState* state);

//----------------------------------------------------------------------------//

static void _draw_start_command_buffer(DrawState* s, VkCommandBuffer commandBuffer);
static void _draw_end_command_buffer(VkCommandBuffer commandBuffer);

static void _draw_record_gparticle_compute_command_buffer(DrawState* s, DrawParams* params, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx);

static void _draw_record_render_pass_command_buffer(DrawState* s, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx);
static void _draw_record_gparticle_graphics_command_buffer(DrawState* s, DrawParams* params, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx);
static void _draw_record_grid_command_buffer(DrawState* s, DrawParams* params, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx);

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

	// create render state:
	//---------------
	if (!vkh_init(&s->instance, 1920, 1080, "VulkanCraft"))
	{
		ERROR_LOG("failed to initialize render instance");
		return false;
	}

	// initialize objects for drawing:
	//---------------
	if (!_draw_create_depth_buffer(s))
		return false;

	if (!_draw_create_final_render_pass(s))
		return false;

	if (!_draw_create_framebuffers(s))
		return false;

	if (!_draw_create_command_buffers(s))
		return false;

	if (!_draw_create_sync_objects(s))
		return false;

	if (!_draw_create_camera_buffer(s))
		return false;

	// initialize reusable vertex buffers:
	//---------------
	if (!_draw_create_quad_vertex_buffer(s))
		return false;

	// initialize grid drawing objects:
	//---------------
	if (!_draw_create_grid_pipeline(s))
		return false;

	if (!_draw_create_grid_descriptors(s))
		return false;

	// initialize galaxy particle drawing objects:
	//---------------
	if (!_draw_create_gparticle_graphics_pipeline(s))
		return false;

	if(!_draw_create_gparticle_compute_pipeline(s))
		return false;

	if(!_draw_create_gparticle_buffer(s))
		return false;

	if (!_draw_create_gparticle_graphics_descriptors(s))
		return false;

	if(!_draw_create_gparticle_compute_descriptors(s))
		return false;

	return true;
}

void draw_quit(DrawState* s)
{
	vkDeviceWaitIdle(s->instance->device);

	_draw_destroy_gparticle_compute_descriptors(s);
	_draw_destroy_gparticle_graphics_descriptors(s);
	_draw_destroy_gparticle_buffer(s);
	_draw_destroy_gparticle_compute_pipeline(s);
	_draw_destroy_gparticle_graphics_pipeline(s);

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

void draw_render(DrawState* s, DrawParams* params)
{
	static uint32 frameIdx = 0;

	// WAIT FOR IN-FLIGHT FENCES AND GET NEXT SWAPCHAIN IMAGE (essentially just making sure last frame is done):
	//---------------
	vkWaitForFences(s->instance->device, 1, &s->inFlightFences[frameIdx], VK_TRUE, UINT64_MAX);

	uint32 imageIdx;
	VkResult imageAquireResult = vkAcquireNextImageKHR(s->instance->device, s->instance->swapchain, UINT64_MAX,
													   s->imageAvailableSemaphores[frameIdx], VK_NULL_HANDLE, &imageIdx);
	if (imageAquireResult == VK_ERROR_OUT_OF_DATE_KHR || imageAquireResult == VK_SUBOPTIMAL_KHR)
	{
		_draw_window_resized(s);
		return;
	}
	else if (imageAquireResult != VK_SUCCESS)
	{
		ERROR_LOG("failed to acquire swapchain image");
		return;
	}

	vkResetFences(s->instance->device, 1, &s->inFlightFences[frameIdx]);

	// UPDATE CAMERA BUFFER:
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

	// RECORD COMMAND BUFFER:
	//---------------
	vkResetCommandBuffer(s->commandBuffers[frameIdx], 0);

	_draw_start_command_buffer(s, s->commandBuffers[frameIdx]);

	_draw_record_gparticle_compute_command_buffer(s, params, s->commandBuffers[frameIdx], frameIdx, imageIdx);

	_draw_record_render_pass_command_buffer(s, s->commandBuffers[frameIdx], frameIdx, imageIdx);
	_draw_record_gparticle_graphics_command_buffer(s, params, s->commandBuffers[frameIdx], frameIdx, imageIdx);
	_draw_record_grid_command_buffer(s, params, s->commandBuffers[frameIdx], frameIdx, imageIdx);

	_draw_end_command_buffer(s->commandBuffers[frameIdx]);

	// SUBMIT COMMAND BUFFERS:
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

	// PRESENT RESULT TO SCREEN:
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
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
		_draw_window_resized(s);
	else if (presentResult != VK_SUCCESS)
		ERROR_LOG("failed to present swapchain image");

	frameIdx = (frameIdx + 1) % FRAMES_IN_FLIGHT;
}

//----------------------------------------------------------------------------//

static bool _draw_create_depth_buffer(DrawState* s)
{
	const uint32 possibleDepthFormatCount = 3;
	const VkFormat possibleDepthFormats[3] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};

	bool depthFormatFound = false;
	for (int32 i = 0; i < possibleDepthFormatCount; i++)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(s->instance->physicalDevice, possibleDepthFormats[i], &properties);

		if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			s->depthFormat = possibleDepthFormats[i];
			depthFormatFound = true;
			break;
		}
	}

	if (!depthFormatFound)
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
	// create attachments:
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

	// create attachment references:
	//---------------
	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference = {};
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// create subpass description:
	//---------------
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;
	subpass.pDepthStencilAttachment = &depthAttachmentReference;

	// create dependency:
	//---------------
	VkSubpassDependency attachmentDependency = {};
	attachmentDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	attachmentDependency.dstSubpass = 0;
	attachmentDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	attachmentDependency.srcAccessMask = 0;
	attachmentDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	attachmentDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency computeDependency = {};
	computeDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	computeDependency.dstSubpass = 0;
	computeDependency.srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	computeDependency.srcAccessMask = 0;
	computeDependency.dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	computeDependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	VkSubpassDependency dependencies[2] = {attachmentDependency, computeDependency};

	// create render pass:
	//---------------
	const uint32 attachmentCount = 2;
	VkAttachmentDescription attachments[2] = {colorAttachment, depthAttachment};

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = attachmentCount;
	renderPassCreateInfo.pAttachments = attachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 2;
	renderPassCreateInfo.pDependencies = dependencies;

	if (vkCreateRenderPass(s->instance->device, &renderPassCreateInfo, nullptr, &s->finalRenderPass) != VK_SUCCESS)
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
	s->framebuffers = (VkFramebuffer *)malloc(s->framebufferCount * sizeof(VkFramebuffer));

	for (uint32 i = 0; i < s->framebufferCount; i++)
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

		if (vkCreateFramebuffer(s->instance->device, &createInfo, nullptr, &s->framebuffers[i]) != VK_SUCCESS)
		{
			ERROR_LOG("failed to create framebuffer");
			return false;
		}
	}

	return true;
}

static void _draw_destroy_framebuffers(DrawState* s)
{
	for (uint32 i = 0; i < s->framebufferCount; i++)
		vkDestroyFramebuffer(s->instance->device, s->framebuffers[i], NULL);

	free(s->framebuffers);
}

static bool _draw_create_command_buffers(DrawState* s)
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = s->instance->graphicsComputeFamilyIdx;

	if (vkCreateCommandPool(s->instance->device, &poolInfo, nullptr, &s->commandPool) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create command pool");
		return false;
	}

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = s->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = FRAMES_IN_FLIGHT;

	if (vkAllocateCommandBuffers(s->instance->device, &allocInfo, s->commandBuffers) != VK_SUCCESS)
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

	for (int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
		if (vkCreateSemaphore(s->instance->device, &semaphoreInfo, NULL, &s->imageAvailableSemaphores[i]) != VK_SUCCESS ||
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
	for (int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(s->instance->device, s->imageAvailableSemaphores[i], NULL);
		vkDestroySemaphore(s->instance->device, s->renderFinishedSemaphores[i], NULL);
		vkDestroyFence(s->instance->device, s->inFlightFences[i], NULL);
	}
}

static bool _draw_create_camera_buffer(DrawState* s)
{
	VkDeviceSize bufferSize = sizeof(CameraGPU);

	for (int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
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

	for (int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
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
	for (int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
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

static bool _draw_create_gparticle_graphics_pipeline(DrawState* s)
{
	//create pipeline object:
	//---------------
	s->particlePipeline = vkh_pipeline_create();
	if(!s->particlePipeline)
		return false;

	//set shaders:
	//---------------
	uint64 vertCodeSize, fragCodeSize;
	uint32 *vertCode = vkh_load_spirv("assets/spirv/galaxy_particle.vert.spv", &vertCodeSize);
	uint32 *fragCode = vkh_load_spirv("assets/spirv/galaxy_particle.frag.spv", &fragCodeSize);

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

	VkDescriptorSetLayoutBinding perInstanceLayoutBinding = {};
	perInstanceLayoutBinding.binding = 1;
	perInstanceLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	perInstanceLayoutBinding.descriptorCount = 1;
	perInstanceLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	perInstanceLayoutBinding.pImmutableSamplers = nullptr;

	vkh_pipeline_add_desc_set_binding(s->particlePipeline, cameraLayoutBinding);
	vkh_pipeline_add_desc_set_binding(s->particlePipeline, perInstanceLayoutBinding);

	//add dynamic states:
	//---------------
	vkh_pipeline_add_dynamic_state(s->particlePipeline, VK_DYNAMIC_STATE_VIEWPORT);
	vkh_pipeline_add_dynamic_state(s->particlePipeline, VK_DYNAMIC_STATE_SCISSOR);

	//add vertex input info:
	//---------------
	VkVertexInputBindingDescription vertBindingDescription = {};
	vertBindingDescription.binding = 0;
	vertBindingDescription.stride = sizeof(Vertex);
	vertBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	vkh_pipeline_add_vertex_input_binding(s->particlePipeline, vertBindingDescription);

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

	vkh_pipeline_add_vertex_input_attrib(s->particlePipeline, vertPositionAttrib);
	vkh_pipeline_add_vertex_input_attrib(s->particlePipeline, vertTexCoordAttrib);

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

	vkh_pipeline_add_color_blend_attachment(s->particlePipeline, colorBlendAttachment);

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

	vkh_pipeline_add_push_constant(s->particlePipeline, vertPushConstant);
	vkh_pipeline_add_push_constant(s->particlePipeline, fragPushConstant);

	//set states:
	//---------------
	vkh_pipeline_set_input_assembly_state(s->particlePipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

	vkh_pipeline_set_raster_state(s->particlePipeline, VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
		VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, 0.0f, 0.0f, 0.0f);

	vkh_pipeline_set_multisample_state(s->particlePipeline, VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 1.0f, NULL, VK_FALSE, VK_FALSE);

	vkh_pipeline_set_depth_stencil_state(s->particlePipeline, VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, VK_FALSE, {}, {}, 0.0f, 1.0f);

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

static void _draw_destroy_gparticle_graphics_pipeline(DrawState* s)
{
	vkh_pipeline_cleanup(s->particlePipeline, s->instance);
	vkh_pipeline_destroy(s->particlePipeline);
}

static bool _draw_create_gparticle_compute_pipeline(DrawState* s)
{
	// create descriptor set layout:
	//---------------
	VkDescriptorSetLayoutBinding particleLayoutBinding = {};
	particleLayoutBinding.binding = 0;
	particleLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	particleLayoutBinding.descriptorCount = 1;
	particleLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	particleLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &particleLayoutBinding;

	if (vkCreateDescriptorSetLayout(s->instance->device, &layoutInfo, nullptr, &s->gparticleComputePipelineDescriptorLayout) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create descriptor set layout for galaxy particle compute pipeline");
		return false;
	}

	// compile:
	//---------------
	uint64 computeCodeSize;
	uint32 *computeCode = vkh_load_spirv("assets/spirv/galaxy_particle.comp.spv", &computeCodeSize);

	// create shader modules:
	//---------------
	VkShaderModule computeModule = vkh_create_shader_module(s->instance, computeCodeSize, computeCode);

	vkh_free_spirv(computeCode);

	// create shader stages:
	//---------------
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
	shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStageCreateInfo.module = computeModule;
	shaderStageCreateInfo.pName = "main";

	// create push constants:
	//---------------
	VkPushConstantRange pushConstants = {};
	pushConstants.offset = 0;
	pushConstants.size = sizeof(ParticleComputeParamsGPU);
	pushConstants.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	// create pipeline layout:
	//---------------
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &s->gparticleComputePipelineDescriptorLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstants;

	if (vkCreatePipelineLayout(s->instance->device, &pipelineLayoutCreateInfo, nullptr, &s->gparticleComputePipelineLayout) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create galaxy particle compute pipeline layout");
		return false;
	}

	// create pipeline:
	//---------------
	VkComputePipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = s->gparticleComputePipelineLayout;
	pipelineCreateInfo.stage = shaderStageCreateInfo;

	if (vkCreateComputePipelines(s->instance->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &s->gparticleComputePipeline) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create galaxy compute particle pipeline");
		return false;
	}

	// free shader modules:
	//---------------
	vkh_destroy_shader_module(s->instance, computeModule);

	return true;
}

static void _draw_destroy_gparticle_compute_pipeline(DrawState* s)
{
	vkDestroyPipeline(s->instance->device, s->gparticleComputePipeline, NULL);
	vkDestroyPipelineLayout(s->instance->device, s->gparticleComputePipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(s->instance->device, s->gparticleComputePipelineDescriptorLayout, NULL);
}

static bool _draw_create_gparticle_buffer(DrawState* s)
{
	s->numGparticles = 0;
	s->gparticleBufferSize = 1024 * sizeof(GalaxyParticle); //default allocate 1024 elements, will resize if necessary

	s->gparticleBuffer = vkh_create_buffer(s->instance, s->gparticleBufferSize,
												VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
												VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &s->gparticleBufferMemory);

	return true;
}

static void _draw_destroy_gparticle_buffer(DrawState* s)
{
	vkh_destroy_buffer(s->instance, s->gparticleBuffer, s->gparticleBufferMemory);
}

static bool _draw_create_gparticle_graphics_descriptors(DrawState* s)
{
	s->particleDescriptorSets = vkh_descriptor_sets_create(FRAMES_IN_FLIGHT);
	if(!s->particleDescriptorSets)
		return false;

	VkDescriptorBufferInfo cameraBufferInfos[FRAMES_IN_FLIGHT];
	VkDescriptorBufferInfo particleBufferInfos[FRAMES_IN_FLIGHT];
	for (int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		cameraBufferInfos[i].buffer = s->cameraBuffers[i];
		cameraBufferInfos[i].offset = 0;
		cameraBufferInfos[i].range = sizeof(CameraGPU);

		particleBufferInfos[i].buffer = s->gparticleBuffer;
		particleBufferInfos[i].offset = 0;
		particleBufferInfos[i].range = VK_WHOLE_SIZE;

		vkh_descriptor_sets_add_buffers(s->particleDescriptorSets, i, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
			0, 0, 1, &cameraBufferInfos[i]);

		vkh_descriptor_sets_add_buffers(s->particleDescriptorSets, i, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 
			1, 0, 1, &particleBufferInfos[i]);
	}

	return vkh_desctiptor_sets_generate(s->particleDescriptorSets, s->instance, s->particlePipeline->descriptorLayout);
}

static void _draw_destroy_gparticle_graphics_descriptors(DrawState* s)
{
	vkh_descriptor_sets_cleanup(s->particleDescriptorSets, s->instance);
	vkh_descriptor_sets_destroy(s->particleDescriptorSets);
}

static bool _draw_create_gparticle_compute_descriptors(DrawState* s)
{
	s->particleComputeDescriptorSets = vkh_descriptor_sets_create(1);
	if(!s->particleComputeDescriptorSets)
		return false;

	VkDescriptorBufferInfo particleBufferInfo = {};
	particleBufferInfo.buffer = s->gparticleBuffer;
	particleBufferInfo.offset = 0;
	particleBufferInfo.range = VK_WHOLE_SIZE;

	vkh_descriptor_sets_add_buffers(s->particleComputeDescriptorSets, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
		0, 0, 1, &particleBufferInfo);
	
	return vkh_desctiptor_sets_generate(s->particleComputeDescriptorSets, s->instance, s->gparticleComputePipelineDescriptorLayout);
}

static void _draw_destroy_gparticle_compute_descriptors(DrawState* s)
{
	vkh_descriptor_sets_cleanup(s->particleComputeDescriptorSets, s->instance);
	vkh_descriptor_sets_destroy(s->particleComputeDescriptorSets);
}

//----------------------------------------------------------------------------//

static void _draw_start_command_buffer(DrawState* s, VkCommandBuffer commandBuffer)
{
	//command buffer begin:
	//---------------
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
}

static void _draw_record_render_pass_command_buffer(DrawState* s, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx)
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

static void _draw_end_command_buffer(VkCommandBuffer commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer);

	if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		ERROR_LOG("failed to end command buffer");
}

static void _draw_record_grid_command_buffer(DrawState* s, DrawParams* params, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx)
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

static void _draw_record_gparticle_compute_command_buffer(DrawState* s, DrawParams* params, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx)
{
	//write barrier: (ensures compute shader will not run while buffer is being read from)
	//---------------
	VkBufferMemoryBarrier writeBarrier = {};
	writeBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	writeBarrier.srcAccessMask = 0;
	writeBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	writeBarrier.srcQueueFamilyIndex = s->instance->graphicsComputeFamilyIdx;
	writeBarrier.dstQueueFamilyIndex = s->instance->graphicsComputeFamilyIdx;
	writeBarrier.buffer = s->gparticleBuffer;
	writeBarrier.offset = 0;
	writeBarrier.size = VK_WHOLE_SIZE;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
							VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 1, &writeBarrier, 0, NULL);

	//dispatch:
	//---------------
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, s->gparticleComputePipeline);

	uint32 dynamicOffset = 0;
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, s->gparticleComputePipelineLayout, 0, 1, &s->particleComputeDescriptorSets->sets[0], 1, &dynamicOffset);

	ParticleComputeParamsGPU computeParams = { (f32)glfwGetTime() };
	vkCmdPushConstants(commandBuffer, s->gparticleComputePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ParticleComputeParamsGPU), &computeParams);

	vkCmdDispatch(commandBuffer, 1, 1, 1);
}

static void _draw_record_gparticle_graphics_command_buffer(DrawState* s, DrawParams* params, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s->particlePipeline->pipeline);

	//bind buffers:
	//---------------
	VkBuffer vertexBuffers[] = {s->quadVertexBuffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, s->quadIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

	uint32 dynamicOffset = 0;
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s->particlePipeline->layout, 0, 1, &s->particleDescriptorSets->sets[frameIndex], 1, &dynamicOffset);

	//draw:
	//---------------
	vkCmdDrawIndexed(commandBuffer, 6, params->numParticles, 0, 0, 0);
}

//----------------------------------------------------------------------------//

static void _draw_window_resized(DrawState* s)
{
	int32 w, h;
	glfwGetFramebufferSize(s->instance->window, &w, &h);
	if (w == 0 || h == 0)
		return;

	vkh_resize_swapchain(s->instance, w, h);

	_draw_destroy_depth_buffer(s);
	_draw_create_depth_buffer(s);

	_draw_destroy_framebuffers(s);
	_draw_create_framebuffers(s);
}

//----------------------------------------------------------------------------//

static void _draw_message_log(const char *message, const char *file, int32 line)
{
	printf("RENDER MESSAGE in %s at line %i - \"%s\"\n\n", file, line, message);
}

static void _draw_error_log(const char *message, const char *file, int32 line)
{
	printf("RENDER ERROR in %s at line %i - \"%s\"\n\n", file, line, message);
}