#include "game_draw.hpp"
#include <malloc.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define MIN_TERRAIN_MEM_BLOCK_SIZE 32
#define MAX_TERRAIN_MEM_BLOCK_SIZE 4194304

//----------------------------------------------------------------------------//

static bool _gamedraw_create_depth_buffer(DrawState* state);
static void _gamedraw_destroy_depth_buffer(DrawState* state);

static bool _gamedraw_create_final_render_pass(DrawState* state);
static void _gamedraw_destroy_final_render_pass(DrawState* state);

static bool _gamedraw_create_framebuffers(DrawState* state);
static void _gamedraw_destroy_framebuffers(DrawState* state);

static bool _gamedraw_create_command_buffers(DrawState* state);
static void _gamedraw_destroy_command_buffers(DrawState* state);

static bool _gamedraw_create_sync_objects(DrawState* state);
static void _gamedraw_destroy_sync_objects(DrawState* state);

//----------------------------------------------------------------------------//

static bool _gamedraw_create_terrain_pipeline(DrawState* state);
static void _gamedraw_destroy_terrain_pipeline(DrawState* state);

static bool _gamedraw_create_terrain_vertex_buffer(DrawState* state, uint64 numVertices);
static void _gamedraw_destroy_terrain_vertex_buffer(DrawState* state);

static bool _gamedraw_create_terrain_storage_buffer(DrawState* state);
static void _gamedraw_destroy_terrain_uniform_buffer(DrawState* state);

static bool _gamedraw_create_terrain_texture_atlas(DrawState* state);
static void _gamedraw_destroy_terrain_texture_atlas(DrawState* state);

static bool _gamedraw_create_terrain_descriptors(DrawState* state);
static void _gamedraw_destroy_terrain_descriptors(DrawState* state);

//----------------------------------------------------------------------------//

static void _gamedraw_record_terrain_command_buffer(DrawState* s, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx);

//----------------------------------------------------------------------------//

static void _gamedraw_message_log(const char* message, const char* file, int32 line);
#define MSG_LOG(m) _gamedraw_message_log(m, __FILE__, __LINE__)

static void _gamedraw_error_log(const char* message, const char* file, int32 line);
#define ERROR_LOG(m) _gamedraw_error_log(m, __FILE__, __LINE__)

//----------------------------------------------------------------------------//

bool gamedraw_init(DrawState** state)
{
	*state = (DrawState*)malloc(sizeof(DrawState));
	DrawState* s = *state;

	//create render state:
	//---------------
	if(!render_init(&s->instance, 1920, 1080, "VulkanCraft"))
	{
		ERROR_LOG("failed to initialize render instance");
		return false;
	}

	//initialize objects for drawing:
	//---------------
	if(!_gamedraw_create_depth_buffer(s))
		return false;

	if(!_gamedraw_create_final_render_pass(s))
		return false;

	if(!_gamedraw_create_framebuffers(s))
		return false;

	if(!_gamedraw_create_command_buffers(s))
		return false;

	if(!_gamedraw_create_sync_objects(s))
		return false;

	//initialize terrain drawing objects:
	//---------------
	if(!_gamedraw_create_terrain_pipeline(s))
		return false;

	if(!_gamedraw_create_terrain_vertex_buffer(s))
		return false;

	if(!_gamedraw_create_terrain_storage_buffer(s))
		return false;

	if(!_gamedraw_create_terrain_texture_atlas(s))
		return false;

	if(!_gamedraw_create_terrain_descriptors(s))
		return false;

	return true;
}

void gamedraw_quit(DrawState* s)
{
	_gamedraw_destroy_terrain_descriptors(s);
	_gamedraw_destroy_terrain_texture_atlas(s);
	_gamedraw_destroy_terrain_uniform_buffer(s);
	_gamedraw_destroy_terrain_vertex_buffer(s);
	_gamedraw_destroy_terrain_pipeline(s);

	_gamedraw_destroy_sync_objects(s);
	_gamedraw_destroy_command_buffers(s);
	_gamedraw_destroy_framebuffers(s);
	_gamedraw_destroy_final_render_pass(s);
	_gamedraw_destroy_depth_buffer(s);

	render_quit(s->instance);

	free(s);
}

//----------------------------------------------------------------------------//

void gamedraw_draw(DrawState* s)
{
	static uint32 frameIndex = 0;

	vkWaitForFences(s->instance->device, 1, &s->inFlightFences[frameIndex], VK_TRUE, UINT64_MAX);

	uint32 imageIdx;
	VkResult imageAquireResult = vkAcquireNextImageKHR(s->instance->device, s->instance->swapchain, UINT64_MAX, 
		s->imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIdx);
	/*if(imageAquireResult == VK_ERROR_OUT_OF_DATE_KHR || imageAquireResult == VK_SUBOPTIMAL_KHR)
	{
		_recreate_vk_swap_chain();
		return;
	}
	else if(imageAquireResult != VK_SUCCESS)
		throw std::runtime_error("FAILED TO ACQUIRE SWAP CHAIN IMAGE");*/

	vkResetFences(s->instance->device, 1, &s->inFlightFences[frameIndex]);

	//_update_uniform_buffer(s, g_frameIndex);

	vkResetCommandBuffer(s->commandBuffers[frameIndex], 0);
	_gamedraw_record_terrain_command_buffer(s, s->commandBuffers[frameIndex], frameIndex, imageIdx);

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

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &s->renderFinishedSemaphores[frameIndex];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &s->instance->swapchain;
	presentInfo.pImageIndices = &imageIdx;
	presentInfo.pResults = nullptr;

	VkResult presentResult = vkQueuePresentKHR(s->instance->presentQueue, &presentInfo);
	if(presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
	{
		//_recreate_vk_swap_chain();
		//g_framebufferResized = false;
	}
	/*else if(presentResult != VK_SUCCESS)
		throw std::runtime_error("FAILED TO PRESENT SWAP CHAIN IMAGE");*/

	frameIndex = (frameIndex + 1) % FRAMES_IN_FLIGHT;
}

//----------------------------------------------------------------------------//

void gamedraw_add_terrain_mesh(DrawState* state, TerrainMesh mesh)
{

}

void gamedraw_remove_terrain_mesh(DrawState* state, uint64 meshID)
{

}

//----------------------------------------------------------------------------//

static bool _gamedraw_create_depth_buffer(DrawState* s)
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

	s->finalDepthImage = render_create_image(s->instance, s->instance->swapchainExtent.width, s->instance->swapchainExtent.height, 1,
		VK_SAMPLE_COUNT_1_BIT, s->depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &s->finalDepthMemory);
	s->finalDepthView = render_create_image_view(s->instance, s->finalDepthImage, s->depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	return true;
}

static void _gamedraw_destroy_depth_buffer(DrawState* s)
{
	render_destroy_image_view(s->instance, s->finalDepthView);
	render_destroy_image(s->instance, s->finalDepthImage, s->finalDepthMemory);
}

static bool _gamedraw_create_final_render_pass(DrawState* s)
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
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

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
	renderPassCreateInfo.pDependencies = &dependency;

	if(vkCreateRenderPass(s->instance->device, &renderPassCreateInfo, nullptr, &s->finalRenderPass) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create final render pass");
		return false;
	}

	return true;
}

static void _gamedraw_destroy_final_render_pass(DrawState* s)
{
	vkDestroyRenderPass(s->instance->device, s->finalRenderPass, NULL);
}

static bool _gamedraw_create_framebuffers(DrawState* s)
{
	s->framebufferCount = s->instance->swapchainImageCount;
	s->framebuffers = (VkFramebuffer*)malloc(s->framebufferCount * sizeof(VkFramebuffer));

	for(int32 i = 0; i < s->framebufferCount; i++)
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

static void _gamedraw_destroy_framebuffers(DrawState* s)
{
	for(int32 i = 0; i < s->framebufferCount; i++)
		vkDestroyFramebuffer(s->instance->device, s->framebuffers[i], NULL);

	free(s->framebuffers);
}

static bool _gamedraw_create_command_buffers(DrawState* s)
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = s->instance->graphicsFamilyIdx;

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

static void _gamedraw_destroy_command_buffers(DrawState* s)
{
	vkFreeCommandBuffers(s->instance->device, s->commandPool, FRAMES_IN_FLIGHT, s->commandBuffers);
	vkDestroyCommandPool(s->instance->device, s->commandPool, NULL);
}

static bool _gamedraw_create_sync_objects(DrawState* s)
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

static void _gamedraw_destroy_sync_objects(DrawState* s)
{
	for(int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(s->instance->device, s->imageAvailableSemaphores[i], NULL);
		vkDestroySemaphore(s->instance->device, s->renderFinishedSemaphores[i], NULL);
		vkDestroyFence(s->instance->device, s->inFlightFences[i], NULL);
	}
}

//----------------------------------------------------------------------------//

static bool _gamedraw_create_terrain_pipeline(DrawState* s)
{
	//create descriptor set layout:
	//---------------
	VkDescriptorSetLayoutBinding storageLayoutBinding = {};
	storageLayoutBinding.binding = 0;
	storageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	storageLayoutBinding.descriptorCount = 1;
	storageLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	storageLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding layouts[2] = {storageLayoutBinding, samplerLayoutBinding};

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 2;
	layoutInfo.pBindings = layouts;

	if(vkCreateDescriptorSetLayout(s->instance->device, &layoutInfo, nullptr, &s->terrainPipelineDescriptorLayout) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create descriptor set layout for terrain pipeline");
		return false;
	}

	//compile:
	//---------------
	uint64 vertexCodeSize, fragmentCodeSize;
	uint32* vertexCode   = render_load_spirv("spirv/vertex.vert.spv", &vertexCodeSize);
	uint32* fragmentCode = render_load_spirv("spirv/fragment.frag.spv", &fragmentCodeSize);

	//create shader modules:
	//---------------
	VkShaderModule vertexModule   = render_create_shader_module(s->instance, vertexCodeSize, vertexCode);
	VkShaderModule fragmentModule = render_create_shader_module(s->instance, fragmentCodeSize, fragmentCode);

	render_free_spirv(vertexCode);
	render_free_spirv(fragmentCode);

	//create shader stages:
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

	//create dynamic state:
	//---------------
	const uint32 dynamicStateCount = 2;
	const VkDynamicState dynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
	dynamicStateCreateInfo.pDynamicStates = dynamicStates;

	//create vertex input info:
	//---------------
	VkVertexInputBindingDescription vertexBindingDescription = {};
	vertexBindingDescription.binding = 0;
	vertexBindingDescription.stride = sizeof(TerrainVertex);
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	const uint32 vertexAttributeCount = 3;
	VkVertexInputAttributeDescription vertexAttributeDescriptions[3] = {};
	
	//pos:
	vertexAttributeDescriptions[0].binding = 0;
	vertexAttributeDescriptions[0].location = 0;
	vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions[0].offset = offsetof(TerrainVertex, pos);

	//tex coord:
	vertexAttributeDescriptions[1].binding = 0;
	vertexAttributeDescriptions[1].location = 1;
	vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions[1].offset = offsetof(TerrainVertex, texCoord);

	//normal:
	vertexAttributeDescriptions[2].binding = 0;
	vertexAttributeDescriptions[2].location = 2;
	vertexAttributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions[2].offset = offsetof(TerrainVertex, normal);

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexAttributeCount;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions;

	//create input assembly state:
	//---------------
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	//create viewport state (dynmaic so only need to specify counts):
	//---------------
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.scissorCount = 1;

	//create rasterization state:
	//---------------
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.lineWidth = 1.0f;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE; //TODO: turn this back on
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

	//create multisample state:
	//---------------
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.minSampleShading = 1.0f;
	multisampleStateCreateInfo.pSampleMask = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	//create depth and stencil state:
	//---------------
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
	depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilCreateInfo.stencilTestEnable = VK_FALSE;

	//create color blend state:
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

	//create push constsant ranges:
	//---------------
	VkPushConstantRange pushConstant = {};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(qm::mat4);
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	//create pipeline layout:
	//---------------
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &s->terrainPipelineDescriptorLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstant;

	if(vkCreatePipelineLayout(s->instance->device, &pipelineLayoutCreateInfo, nullptr, &s->terrainPipelineLayout) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create pipeline layout");
		return false;
	}

	//create pipeline:
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
	pipelineCreateInfo.layout = s->terrainPipelineLayout;
	pipelineCreateInfo.renderPass = s->finalRenderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;

	if(vkCreateGraphicsPipelines(s->instance->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &s->terrainPipeline) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create terrain pipeline");
		return false;
	}

	//free shader modules:
	//---------------
	render_destroy_shader_module(s->instance, vertexModule);
	render_destroy_shader_module(s->instance, fragmentModule);

	return true;
}

static void _gamedraw_destroy_terrain_pipeline(DrawState* s)
{
	vkDestroyPipeline(s->instance->device, s->terrainPipeline, NULL);
	vkDestroyPipelineLayout(s->instance->device, s->terrainPipelineLayout, NULL);
	vkDestroyDescriptorSetLayout(s->instance->device, s->terrainPipelineDescriptorLayout, NULL);
}

static bool _gamedraw_create_terrain_vertex_buffer(DrawState* s, uint64 numVertices)
{
	VkDeviceSize bufferSize = numVertices * sizeof(TerrainVertex);

	//create staging buffer:
	VkDeviceMemory stagingBufferMemory;
	VkBuffer stagingBuffer = render_create_buffer(s->instance, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBufferMemory);

	TerrainVertex tempData[3] = { {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}, {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}, {{0.0f, 0.5f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}} };

	//fill staging buffer:
	void* data;
	vkMapMemory(s->instance->device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, tempData, (size_t)bufferSize);
	vkUnmapMemory(s->instance->device, stagingBufferMemory);

	//create dest buffer and copy into it:
	s->terrainVertexBuffer = render_create_buffer(s->instance, bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &s->terrainVertexBufferMemory);
	render_copy_buffer(s->instance, stagingBuffer, s->terrainVertexBuffer, bufferSize, 0, 0);

	//free staging buffer:
	render_destroy_buffer(s->instance, stagingBuffer, stagingBufferMemory);

	//init mem blocks:
	s->terrainVertexMem = (TerrainMemBlock*)malloc(numVertices / MIN_TERRAIN_MEM_BLOCK_SIZE * sizeof(TerrainMemBlock));
	for(int32 i = 0; i < numVertices / MIN_TERRAIN_MEM_BLOCK_SIZE; i++)
	{
		uint32 vertexIdx = i * MIN_TERRAIN_MEM_BLOCK_SIZE;

		s->terrainVertexMem[i].size = MAX_TERRAIN_MEM_BLOCK_SIZE;
		s->terrainVertexMem[i].start = i / MAX_TERRAIN_MEM_BLOCK_SIZE;
		s->terrainVertexMem[i].taken = false;
	}

	return true;
}

static void _gamedraw_destroy_terrain_vertex_buffer(DrawState* s)
{
	render_destroy_buffer(s->instance, s->terrainVertexBuffer, s->terrainVertexBufferMemory);
}

static bool _gamedraw_create_terrain_storage_buffer(DrawState* s)
{
	VkDeviceSize bufferSize = sizeof(qm::mat4);

	for(int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		s->terrainStorageBuffers[i] = render_create_buffer(s->instance, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &s->terrainUniformBufferMemory[i]);
		vkMapMemory(s->instance->device, s->terrainUniformBufferMemory[i], 0, bufferSize, 0, &s->terrainUniformBuffersMapped[i]);
	}

	return true;
}

static void _gamedraw_destroy_terrain_uniform_buffer(DrawState* s)
{
	for(int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		vkUnmapMemory(s->instance->device, s->terrainUniformBufferMemory[i]);
		render_destroy_buffer(s->instance, s->terrainStorageBuffers[i], s->terrainUniformBufferMemory[i]);
	}
}

static bool _gamedraw_create_terrain_texture_atlas(DrawState* s)
{
	//load from disk:
	//---------------
	int32 width, height, numChannels;
	stbi_set_flip_vertically_on_load(true);
	stbi_uc* raw = stbi_load("textures/atlas.jpg", &width, &height, &numChannels, STBI_rgb_alpha);
	if(!raw)
	{
		ERROR_LOG("failed to load texture atlas");
		return false;
	}

	VkDeviceSize imageSize = width * height * 4;

	VkDeviceMemory stagingBufferMemory;
	VkBuffer stagingBuffer = render_create_buffer(s->instance, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBufferMemory);

	//create image:
	//---------------
	void* stagingBufferData;
	vkMapMemory(s->instance->device, stagingBufferMemory, 0, imageSize, 0, &stagingBufferData);
	memcpy(stagingBufferData, raw, (size_t)imageSize);
	vkUnmapMemory(s->instance->device, stagingBufferMemory);

	stbi_image_free(raw);

	s->terrainTextureAtlas = render_create_image(s->instance, width, height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, 
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &s->terrainTextureAtlasMemory);
	render_transition_image_layout(s->instance, s->terrainTextureAtlas, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
	render_copy_buffer_to_image(s->instance, stagingBuffer, s->terrainTextureAtlas, width, height);
	render_transition_image_layout(s->instance, s->terrainTextureAtlas, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

	vkDestroyBuffer(s->instance->device, stagingBuffer, nullptr);
	vkFreeMemory(s->instance->device, stagingBufferMemory, nullptr);

	//create image view:
	//---------------
	s->terrainTextureAtlasView = render_create_image_view(s->instance, s->terrainTextureAtlas, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	//create sampler:
	//---------------
	VkPhysicalDeviceProperties properties = {};
	vkGetPhysicalDeviceProperties(s->instance->physicalDevice, &properties);

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;

	if(vkCreateSampler(s->instance->device, &samplerInfo, nullptr, &s->terrainTextureAtlasSampler) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create texture atlas sampler");
		return false;
	}

	return true;
}

static void _gamedraw_destroy_terrain_texture_atlas(DrawState* s)
{
	vkDestroySampler(s->instance->device, s->terrainTextureAtlasSampler, NULL);
	render_destroy_image_view(s->instance, s->terrainTextureAtlasView);
	render_destroy_image(s->instance, s->terrainTextureAtlas, s->terrainTextureAtlasMemory);
}

static bool _gamedraw_create_terrain_descriptors(DrawState* s)
{
	VkDescriptorPoolSize poolSizes[2] = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[0].descriptorCount = FRAMES_IN_FLIGHT;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = FRAMES_IN_FLIGHT;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = FRAMES_IN_FLIGHT;

	if(vkCreateDescriptorPool(s->instance->device, &poolInfo, nullptr, &s->terrainDescriptorPool) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create descriptor pool");
		return false;
	}

	VkDescriptorSetLayout layouts[FRAMES_IN_FLIGHT];
	for(int32 i = 0; i < FRAMES_IN_FLIGHT; i++)
		layouts[i] = s->terrainPipelineDescriptorLayout;

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = s->terrainDescriptorPool;
	allocInfo.descriptorSetCount = FRAMES_IN_FLIGHT;
	allocInfo.pSetLayouts = layouts;

	if(vkAllocateDescriptorSets(s->instance->device, &allocInfo, s->terrainDescriptorSets) != VK_SUCCESS)
	{
		ERROR_LOG("failed to allocate descriptor sets");
		return false;
	}

	for(int i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = s->terrainStorageBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(qm::mat4); //TODO: figure out if this can be set dynamically?

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = s->terrainTextureAtlasView;
		imageInfo.sampler = s->terrainTextureAtlasSampler;

		VkWriteDescriptorSet descriptorWrites[2] = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = s->terrainDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = s->terrainDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(s->instance->device, 2, descriptorWrites, 0, nullptr);
	}

	return true;
}

static void _gamedraw_destroy_terrain_descriptors(DrawState* s)
{
	vkDestroyDescriptorPool(s->instance->device, s->terrainDescriptorPool, NULL);
}

//----------------------------------------------------------------------------//

static void _gamedraw_record_terrain_command_buffer(DrawState* s, VkCommandBuffer commandBuffer, uint32 frameIndex, uint32 imageIdx)
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
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s->terrainPipeline);

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

	VkBuffer vertexBuffers[] = {s->terrainVertexBuffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets); 

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s->terrainPipelineLayout, 0, 1, &s->terrainDescriptorSets[frameIndex], 0, nullptr);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);

	vkEndCommandBuffer(commandBuffer) != VK_SUCCESS;
}

//----------------------------------------------------------------------------//

static void _gamedraw_message_log(const char* message, const char* file, int32 line)
{
	printf("RENDER MESSAGE in %s at line %i - \"%s\"\n\n", file, line, message);
}

static void _gamedraw_error_log(const char* message, const char* file, int32 line)
{
	printf("RENDER ERROR in %s at line %i - \"%s\"\n\n", file, line, message);
}