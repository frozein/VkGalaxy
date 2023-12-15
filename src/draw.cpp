#include "draw.hpp"
#include <malloc.h>
#include <stdio.h>
#include <string.h>

//----------------------------------------------------------------------------//

// mirrors camera buffer on GPU
struct CameraGPU
{
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
	int32 numCells;
	float thickness;
	float scroll;
};

//----------------------------------------------------------------------------//

static bool _draw_create_depth_buffer(DrawState *state);
static void _draw_destroy_depth_buffer(DrawState *state);

static bool _draw_create_final_render_pass(DrawState *state);
static void _draw_destroy_final_render_pass(DrawState *state);

static bool _draw_create_framebuffers(DrawState *state);
static void _draw_destroy_framebuffers(DrawState *state);

static bool _draw_create_command_buffers(DrawState *state);
static void _draw_destroy_command_buffers(DrawState *state);

static bool _draw_create_sync_objects(DrawState *state);
static void _draw_destroy_sync_objects(DrawState *state);

static bool _draw_create_camera_buffer(DrawState *state);
static void _draw_destroy_camera_buffer(DrawState *state);

//----------------------------------------------------------------------------//

static bool _draw_create_grid_pipeline(DrawState *state);
static void _draw_destroy_grid_pipeline(DrawState *state);

static bool _draw_create_grid_vertex_buffer(DrawState *state);
static void _draw_destroy_grid_vertex_buffer(DrawState *state);

static bool _draw_create_grid_descriptors(DrawState *state);
static void _draw_destroy_grid_descriptors(DrawState *state);

//----------------------------------------------------------------------------//

static void _draw_record_grid_command_buffer(DrawState *s, Camera* cam, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx);

//----------------------------------------------------------------------------//

static void _draw_window_resized(DrawState *state);

//----------------------------------------------------------------------------//

static void _draw_message_log(const char *message, const char *file, int32 line);
#define MSG_LOG(m) _draw_message_log(m, __FILE__, __LINE__)

static void _draw_error_log(const char *message, const char *file, int32 line);
#define ERROR_LOG(m) _draw_error_log(m, __FILE__, __LINE__)

//----------------------------------------------------------------------------//

bool draw_init(DrawState **state)
{
	*state = (DrawState *)malloc(sizeof(DrawState));
	DrawState *s = *state;

	// create render state:
	//---------------
	if (!render_init(&s->instance, 1920, 1080, "VulkanCraft"))
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

	// initialize grid drawing objects:
	//---------------
	if (!_draw_create_grid_pipeline(s))
		return false;

	if (!_draw_create_grid_vertex_buffer(s))
		return false;

	if (!_draw_create_grid_descriptors(s))
		return false;

	return true;
}

void draw_quit(DrawState *s)
{
	vkDeviceWaitIdle(s->instance->device);

	_draw_destroy_grid_descriptors(s);
	_draw_destroy_grid_vertex_buffer(s);
	_draw_destroy_grid_pipeline(s);

	_draw_destroy_camera_buffer(s);
	_draw_destroy_sync_objects(s);
	_draw_destroy_command_buffers(s);
	_draw_destroy_framebuffers(s);
	_draw_destroy_final_render_pass(s);
	_draw_destroy_depth_buffer(s);

	render_quit(s->instance);

	free(s);
}

//----------------------------------------------------------------------------//

void draw_render(DrawState *s, Camera *cam)
{
	static uint32 frameIndex = 0;

	// WAIT FOR IN-FLIGHT FENCES AND GET NEXT SWAPCHAIN IMAGE (essentially just making sure last frame is done):
	//---------------
	vkWaitForFences(s->instance->device, 1, &s->inFlightFences[frameIndex], VK_TRUE, UINT64_MAX);

	uint32 imageIdx;
	VkResult imageAquireResult = vkAcquireNextImageKHR(s->instance->device, s->instance->swapchain, UINT64_MAX,
													   s->imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIdx);
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

	vkResetFences(s->instance->device, 1, &s->inFlightFences[frameIndex]);

	// UPDATE CAMERA BUFFER:
	//---------------
	int32 windowW, windowH;
	glfwGetWindowSize(s->instance->window, &windowW, &windowH);

	qm::mat4 view, projection;
	view = qm::lookat(cam->pos, cam->center, cam->up);
	projection = qm::perspective(cam->fov, (float)windowW / (float)windowH, cam->nearPlane, 2.0f * cam->dist);

	CameraGPU camBuffer;
	camBuffer.viewProj = projection * view;
	render_upload_with_staging_buffer(s->instance, s->cameraStagingBuffer, s->cameraStagingBufferMemory,
									  s->cameraBuffers[frameIndex], sizeof(CameraGPU), 0, &camBuffer);

	// RECORD GRID COMMAND BUFFER:
	//---------------
	vkResetCommandBuffer(s->commandBuffers[frameIndex], 0);
	_draw_record_grid_command_buffer(s, cam, s->commandBuffers[frameIndex], frameIndex, imageIdx);

	// SUBMIT COMMAND BUFFERS:
	//---------------
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &s->imageAvailableSemaphores[frameIndex];
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &s->commandBuffers[frameIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &s->renderFinishedSemaphores[frameIndex];

	vkQueueSubmit(s->instance->graphicsQueue, 1, &submitInfo, s->inFlightFences[frameIndex]);

	// PRESENT RESULT TO SCREEN:
	//---------------
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &s->renderFinishedSemaphores[frameIndex];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &s->instance->swapchain;
	presentInfo.pImageIndices = &imageIdx;
	presentInfo.pResults = nullptr;

	VkResult presentResult = vkQueuePresentKHR(s->instance->presentQueue, &presentInfo);
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
		_draw_window_resized(s);
	else if (presentResult != VK_SUCCESS)
		ERROR_LOG("failed to present swapchain image");

	frameIndex = (frameIndex + 1) % FRAMES_IN_FLIGHT;
}

//----------------------------------------------------------------------------//

static bool _draw_create_depth_buffer(DrawState *s)
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

	s->finalDepthImage = render_create_image(s->instance, s->instance->swapchainExtent.width, s->instance->swapchainExtent.height, 1,
											 VK_SAMPLE_COUNT_1_BIT, s->depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
											 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &s->finalDepthMemory);
	s->finalDepthView = render_create_image_view(s->instance, s->finalDepthImage, s->depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	return true;
}

static void _draw_destroy_depth_buffer(DrawState *s)
{
	render_destroy_image_view(s->instance, s->finalDepthView);
	render_destroy_image(s->instance, s->finalDepthImage, s->finalDepthMemory);
}

static bool _draw_create_final_render_pass(DrawState *s)
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
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

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
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(s->instance->device, &renderPassCreateInfo, nullptr, &s->finalRenderPass) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create final render pass");
		return false;
	}

	return true;
}

static void _draw_destroy_final_render_pass(DrawState *s)
{
	vkDestroyRenderPass(s->instance->device, s->finalRenderPass, NULL);
}

static bool _draw_create_framebuffers(DrawState *s)
{
	s->framebufferCount = s->instance->swapchainImageCount;
	s->framebuffers = (VkFramebuffer *)malloc(s->framebufferCount * sizeof(VkFramebuffer));

	for (int32 i = 0; i < s->framebufferCount; i++)
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

static void _draw_destroy_framebuffers(DrawState *s)
{
	for (int32 i = 0; i < s->framebufferCount; i++)
		vkDestroyFramebuffer(s->instance->device, s->framebuffers[i], NULL);

	free(s->framebuffers);
}

static bool _draw_create_command_buffers(DrawState *s)
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = s->instance->graphicsFamilyIdx;

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

static void _draw_destroy_command_buffers(DrawState *s)
{
	vkFreeCommandBuffers(s->instance->device, s->commandPool, FRAMES_IN_FLIGHT, s->commandBuffers);
	vkDestroyCommandPool(s->instance->device, s->commandPool, NULL);
}

static bool _draw_create_sync_objects(DrawState *s)
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

static void _draw_destroy_sync_objects(DrawState *s)
{
	for (int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(s->instance->device, s->imageAvailableSemaphores[i], NULL);
		vkDestroySemaphore(s->instance->device, s->renderFinishedSemaphores[i], NULL);
		vkDestroyFence(s->instance->device, s->inFlightFences[i], NULL);
	}
}

static bool _draw_create_camera_buffer(DrawState *s)
{
	VkDeviceSize bufferSize = sizeof(CameraGPU);

	for (int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		s->cameraBuffers[i] = render_create_buffer(s->instance, bufferSize,
												   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
												   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &s->cameraBuffersMemory[i]);
	}

	s->cameraStagingBuffer = render_create_buffer(s->instance, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
												  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &s->cameraStagingBufferMemory);

	return true;
}

static void _draw_destroy_camera_buffer(DrawState *s)
{
	render_destroy_buffer(s->instance, s->cameraStagingBuffer, s->cameraStagingBufferMemory);

	for (int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
		render_destroy_buffer(s->instance, s->cameraBuffers[i], s->cameraBuffersMemory[i]);
}

//----------------------------------------------------------------------------//

static bool _draw_create_grid_pipeline(DrawState *s)
{
	// create descriptor set layout:
	//---------------
	VkDescriptorSetLayoutBinding storageLayoutBinding = {};
	storageLayoutBinding.binding = 0;
	storageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	storageLayoutBinding.descriptorCount = 1;
	storageLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	storageLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding layouts[1] = {storageLayoutBinding};

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = layouts;

	if (vkCreateDescriptorSetLayout(s->instance->device, &layoutInfo, nullptr, &s->gridPipelineDescriptorLayout) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create descriptor set layout for terrain pipeline");
		return false;
	}

	// compile:
	//---------------
	uint64 vertexCodeSize, fragmentCodeSize;
	uint32 *vertexCode = render_load_spirv("assets/spirv/grid.vert.spv", &vertexCodeSize);
	uint32 *fragmentCode = render_load_spirv("assets/spirv/grid.frag.spv", &fragmentCodeSize);

	// create shader modules:
	//---------------
	VkShaderModule vertexModule = render_create_shader_module(s->instance, vertexCodeSize, vertexCode);
	VkShaderModule fragmentModule = render_create_shader_module(s->instance, fragmentCodeSize, fragmentCode);

	render_free_spirv(vertexCode);
	render_free_spirv(fragmentCode);

	// create shader stages:
	//---------------
	VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo = {};
	vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageCreateInfo.module = vertexModule;
	vertShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStagCreateInfo = {};
	fragShaderStagCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStagCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStagCreateInfo.module = fragmentModule;
	fragShaderStagCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageCreateInfo, fragShaderStagCreateInfo};

	// create dynamic state:
	//---------------
	const uint32 dynamicStateCount = 2;
	const VkDynamicState dynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
	dynamicStateCreateInfo.pDynamicStates = dynamicStates;

	// create vertex input info:
	//---------------
	VkVertexInputBindingDescription vertexBindingDescription = {};
	vertexBindingDescription.binding = 0;
	vertexBindingDescription.stride = sizeof(Vertex);
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	const uint32 vertexAttributeCount = 2;
	VkVertexInputAttributeDescription vertexAttributeDescriptions[3] = {};

	// pos:
	vertexAttributeDescriptions[0].binding = 0;
	vertexAttributeDescriptions[0].location = 0;
	vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions[0].offset = offsetof(Vertex, pos);

	// tex coord:
	vertexAttributeDescriptions[1].binding = 0;
	vertexAttributeDescriptions[1].location = 1;
	vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributeDescriptions[1].offset = offsetof(Vertex, texCoord);

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexAttributeCount;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions;

	// create input assembly state:
	//---------------
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	// create viewport state (dynmaic so only need to specify counts):
	//---------------
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.scissorCount = 1;

	// create rasterization state:
	//---------------
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.lineWidth = 1.0f;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE; // TODO: turn this back on
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

	// create multisample state:
	//---------------
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.minSampleShading = 1.0f;
	multisampleStateCreateInfo.pSampleMask = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	// create depth and stencil state:
	//---------------
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
	depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilCreateInfo.stencilTestEnable = VK_FALSE;

	// create color blend state:
	//---------------
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_FALSE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
	colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

	// create push constsant ranges:
	//---------------
	VkPushConstantRange vertPushConstant = {};
	vertPushConstant.offset = 0;
	vertPushConstant.size = sizeof(GridParamsVertGPU);
	vertPushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkPushConstantRange fragPushConstant = {};
	fragPushConstant.offset = sizeof(GridParamsVertGPU);
	fragPushConstant.size = sizeof(GridParamsFragGPU);
	fragPushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPushConstantRange pushConstants[] = {vertPushConstant, fragPushConstant};

	// create pipeline layout:
	//---------------
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &s->gridPipelineDescriptorLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 2;
	pipelineLayoutCreateInfo.pPushConstantRanges = pushConstants;

	if (vkCreatePipelineLayout(s->instance->device, &pipelineLayoutCreateInfo, nullptr, &s->gridPipelineLayout) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create pipeline layout");
		return false;
	}

	// create pipeline:
	//---------------
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	pipelineCreateInfo.layout = s->gridPipelineLayout;
	pipelineCreateInfo.renderPass = s->finalRenderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(s->instance->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &s->gridPipeline) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create terrain pipeline");
		return false;
	}

	// free shader modules:
	//---------------
	render_destroy_shader_module(s->instance, vertexModule);
	render_destroy_shader_module(s->instance, fragmentModule);

	return true;
}

static void _draw_destroy_grid_pipeline(DrawState *s)
{
	vkDestroyPipeline(s->instance->device, s->gridPipeline, NULL);
	vkDestroyPipelineLayout(s->instance->device, s->gridPipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(s->instance->device, s->gridPipelineDescriptorLayout, NULL);
}

static bool _draw_create_grid_vertex_buffer(DrawState *s)
{
	Vertex verts[4] = {{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}}, {{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}}, {{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}}, {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}}};
	uint32 indices[6] = {0, 1, 2, 1, 2, 3};

	s->gridVertexBuffer = render_create_buffer(s->instance, sizeof(verts),
												  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
												  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &s->gridVertexBufferMemory);
	render_upload_with_staging_buffer(s->instance, s->gridVertexBuffer, sizeof(verts), 0, verts);

	s->gridIndexBuffer = render_create_buffer(s->instance, sizeof(indices),
												 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
												 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &s->gridIndexBufferMemory);
	render_upload_with_staging_buffer(s->instance, s->gridIndexBuffer, sizeof(indices), 0, indices);

	return true;
}

static void _draw_destroy_grid_vertex_buffer(DrawState *s)
{
	render_destroy_buffer(s->instance, s->gridVertexBuffer, s->gridVertexBufferMemory);
	render_destroy_buffer(s->instance, s->gridIndexBuffer, s->gridIndexBufferMemory);
}

static bool _draw_create_grid_descriptors(DrawState *s)
{
	VkDescriptorPoolSize poolSizes[1] = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[0].descriptorCount = FRAMES_IN_FLIGHT;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = FRAMES_IN_FLIGHT;

	if (vkCreateDescriptorPool(s->instance->device, &poolInfo, nullptr, &s->gridDescriptorPool) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create descriptor pool");
		return false;
	}

	VkDescriptorSetLayout layouts[FRAMES_IN_FLIGHT];
	for (int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
		layouts[i] = s->gridPipelineDescriptorLayout;

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = s->gridDescriptorPool;
	allocInfo.descriptorSetCount = FRAMES_IN_FLIGHT;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(s->instance->device, &allocInfo, s->gridDescriptorSets) != VK_SUCCESS)
	{
		ERROR_LOG("failed to allocate descriptor sets");
		return false;
	}

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = s->cameraBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(CameraGPU); // TODO: figure out if this can be set dynamically?

		VkWriteDescriptorSet descriptorWrites[1] = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = s->gridDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(s->instance->device, 1, descriptorWrites, 0, nullptr);
	}

	return true;
}

static void _draw_destroy_grid_descriptors(DrawState *s)
{
	vkDestroyDescriptorPool(s->instance->device, s->gridDescriptorPool, NULL);
}

//----------------------------------------------------------------------------//

static void _draw_record_grid_command_buffer(DrawState *s, Camera* cam, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx)
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

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
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s->gridPipeline);

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)s->instance->swapchainExtent.width;
	viewport.height = (float)s->instance->swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = s->instance->swapchainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	//bind buffers:
	//---------------
	VkBuffer vertexBuffers[] = {s->gridVertexBuffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, s->gridIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s->gridPipelineLayout, 0, 1, &s->gridDescriptorSets[frameIndex], 0, nullptr);

	//send fragment stage params:
	//---------------
	int32 numCells = 32;
	float thickness = 0.0125f;
	float scroll = (cam->dist - powf(2.0f, roundf(log2f(cam->dist) - 0.5f))) / (4.0f * powf(2.0f, roundf(log2f(cam->dist) - 1.5f))) + 0.5f;

	GridParamsFragGPU fragParams = {numCells, thickness, scroll};
	vkCmdPushConstants(commandBuffer, s->gridPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(GridParamsVertGPU), sizeof(GridParamsFragGPU), &fragParams);

	//send vertex stage params:
	//---------------
	int32 windowW, windowH;
	glfwGetWindowSize(s->instance->window, &windowW, &windowH);	//TODO: FIGURE OUT WHY IT GETS CUT OFF WITH VERY TALL WINDOWS
	float aspect = (float)windowW / (float)windowH;
	if(aspect < 1.0f)
		aspect = 1.0f / aspect;

	float size = aspect * 3.0f * powf(2.0f, roundf(log2f(cam->dist) + 0.5f));

	qm::vec3 pos = cam->center;
	for(int32 i = 0; i < 3; i++)
		pos[i] -= fmodf(pos[i], size / numCells);

	qm::mat4 model = qm::translate(pos) * qm::scale(qm::vec3(size, size, size));

	GridParamsVertGPU vertParams = {model};
	vkCmdPushConstants(commandBuffer, s->gridPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GridParamsVertGPU), &vertParams);

	//draw:
	//---------------
	vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
	vkCmdEndRenderPass(commandBuffer);

	vkEndCommandBuffer(commandBuffer) != VK_SUCCESS;
}

//----------------------------------------------------------------------------//

static void _draw_window_resized(DrawState *s)
{
	int32 w, h;
	glfwGetFramebufferSize(s->instance->window, &w, &h);
	if (w == 0 || h == 0) // TODO: TEST
		return;

	render_resize_swapchain(s->instance, w, h);

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