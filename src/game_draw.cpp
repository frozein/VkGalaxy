#include "game_draw.hpp"
#include <malloc.h>
#include <stdio.h>

//----------------------------------------------------------------------------//

struct TerrainVertex
{
	f32 x, y, z;
};

//----------------------------------------------------------------------------//

static bool _gamedraw_create_depth_buffer(DrawState* state);
static void _gamedraw_destroy_depth_buffer(DrawState* state);

static bool _gamedraw_create_final_render_pass(DrawState* state);
static void _gamedraw_destroy_final_render_pass(DrawState* state);

static bool _gamedraw_create_terrain_pipeline(DrawState* state);
static void _gamedraw_destroy_terrain_pipeline(DrawState* state);

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

	if(!render_init(&s->instance, 1920, 1080, "VulkanCraft"))
	{
		ERROR_LOG("failed to initialize render instance");
		return false;
	}

	if(!_gamedraw_create_depth_buffer(s))
		return false;

	if(!_gamedraw_create_final_render_pass(s))
		return false;

	if(!_gamedraw_create_terrain_pipeline(s))
		return false;

	return true;
}

void gamedraw_quit(DrawState* s)
{
	_gamedraw_destroy_terrain_pipeline(s);
	_gamedraw_destroy_final_render_pass(s);
	_gamedraw_destroy_depth_buffer(s);
	render_quit(s->instance);

	free(s);
}

//----------------------------------------------------------------------------//

void gamedraw_draw(DrawState* state)
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

static bool _gamedraw_create_terrain_pipeline(DrawState* s)
{
	//create descriptor set layout:
	//---------------
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerBinding = {};
	samplerBinding.binding = 1;
	samplerBinding.descriptorCount = 1;
	samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding layouts[2] = {uboLayoutBinding, samplerBinding};

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
	uint32* vertexCode   = render_load_spirv("shader.vert", &vertexCodeSize);
	uint32* fragmentCode = render_load_spirv("shader.frag", &fragmentCodeSize);

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

	const uint32 vertexAttributeCount = 1;
	VkVertexInputAttributeDescription vertexAttributeDescriptions[1] = {{}};
	vertexAttributeDescriptions[0].binding = 0;
	vertexAttributeDescriptions[0].location = 0;
	vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions[0].offset = 0;

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
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
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

	//create pipeline layout;
	//---------------
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &s->terrainPipelineDescriptorLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

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

//----------------------------------------------------------------------------//

static void _gamedraw_message_log(const char* message, const char* file, int32 line)
{
	printf("RENDER MESSAGE in %s at line %i - \"%s\"\n\n", file, line, message);
}

static void _gamedraw_error_log(const char* message, const char* file, int32 line)
{
	printf("RENDER ERROR in %s at line %i - \"%s\"\n\n", file, line, message);
}