#ifndef VKH_H
#define VKH_H

#ifdef __cplusplus
extern "C" 
{
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdint.h>

#include "./quickdata.h"

#define VKH_VALIDATION_LAYERS 1

//----------------------------------------------------------------------------//

typedef int32_t vkh_bool_t;
#define VKH_TRUE  1
#define VKH_FALSE 0

typedef struct VKHinstance
{
	GLFWwindow* window;

	VkInstance instance;
	VkDevice device;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;

	uint32_t graphicsComputeFamilyIdx;
	uint32_t presentFamilyIdx;
	VkQueue graphicsQueue;
	VkQueue computeQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapchain;
	VkFormat swapchainFormat;
	VkExtent2D swapchainExtent;
	uint32_t swapchainImageCount;
	VkImage* swapchainImages;
	VkImageView* swapchainImageViews;

	VkCommandPool commandPool;

	#if VKH_VALIDATION_LAYERS
		VkDebugUtilsMessengerEXT debugMessenger;
	#endif
} VKHinstance;

typedef struct VKHgraphicsPipeline
{
	//intermediates:
	//---------------
	QDdynArray* descSetBindings;       //type - VkDescriptorSetLayoutBinding
	QDdynArray* dynamicStates;         //type - VkDynamicState
	QDdynArray* vertInputBindings;     //type - VkVertexInputBindingDescription
	QDdynArray* vertInputAttribs;      //type - VkVertexInputAttributeDescription
	QDdynArray* viewports;             //type - VkViewport
	QDdynArray* scissors;              //type - VkRect2D
	QDdynArray* colorBlendAttachments; //type - VkPipelineColorBlendAttachmentState
	QDdynArray* pushConstants;         //type - VkPushConstantRange

	VkShaderModule vertShader;
	VkShaderModule fragShader;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
	VkPipelineTessellationStateCreateInfo tesselationState;
	VkPipelineRasterizationStateCreateInfo rasterState;
	VkPipelineMultisampleStateCreateInfo multisampleState;
	VkPipelineDepthStencilStateCreateInfo depthStencilState;
	VkPipelineColorBlendStateCreateInfo colorBlendState;

	//generated:
	//---------------
	vkh_bool_t generated;

	VkDescriptorSetLayout descriptorLayout;
	VkPipelineLayout layout;
	VkPipeline pipeline;

} VKHgraphicsPipeline;

typedef struct VKHcomputePipeline
{
	//intermediates:
	//---------------
	QDdynArray* descSetBindings; //type - VkDescriptorSetLayoutBinding
	QDdynArray* pushConstants;   //type - VkPushConstantRange

	VkShaderModule shader;

	//generated:
	//---------------
	vkh_bool_t generated;

	VkDescriptorSetLayout descriptorLayout;
	VkPipelineLayout layout;
	VkPipeline pipeline;

} VKHcomputePipeline;

typedef struct VKHdescriptorInfo
{
	uint32_t index;

	uint32_t count;
	union
	{
		VkDescriptorBufferInfo* bufferInfos;
		VkDescriptorImageInfo* imageInfos;
		VkBufferView* texelBufferViews;
	};

	VkDescriptorType type;
	uint32_t binding;
	uint32_t arrayElem;

} VKHdescriptorInfo;

typedef struct VKHdescriptorSets
{
	//intermediate:
	//---------------
	uint32_t count;
	QDdynArray* descriptors; //type - VKHdescriptorInfo

	//generated:
	//---------------
	vkh_bool_t generated;

	VkDescriptorPool pool;
	VkDescriptorSet* sets;
} VKHdescriptorSets;

//----------------------------------------------------------------------------//

vkh_bool_t vkh_init(VKHinstance** instance, uint32_t windowW, uint32_t windowH, const char* windowName);
void       vkh_quit(VKHinstance* instance);

void vkh_resize_swapchain(VKHinstance* instance, uint32_t w, uint32_t h);

//----------------------------------------------------------------------------//

VkImage     vkh_create_image                  (VKHinstance* instance, uint32_t w, uint32_t h, uint32_t mipLevels, VkSampleCountFlagBits samples, 
                                               VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceMemory* memory);
void        vkh_destroy_image                 (VKHinstance* instance, VkImage image, VkDeviceMemory memory);

VkImageView vkh_create_image_view             (VKHinstance* instance, VkImage image, VkFormat format, VkImageAspectFlags aspects, uint32_t mipLevels);
void        vkh_destroy_image_view            (VKHinstance* instance, VkImageView view);

VkBuffer    vkh_create_buffer                 (VKHinstance* instance, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceMemory* memory);
void        vkh_destroy_buffer                (VKHinstance* instance, VkBuffer buffer, VkDeviceMemory memory);

void        vkh_copy_buffer                   (VKHinstance* instance, VkBuffer src, VkBuffer dst, VkDeviceSize size, uint64_t srcOffset, uint64_t dstOffset);
void        vkh_copy_buffer_to_image          (VKHinstance* instance, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

void        vkh_copy_with_staging_buf         (VKHinstance* instance, VkBuffer stagingBuf, VkDeviceMemory stagingBufMem, VkBuffer buf, uint64_t size, uint64_t offset, void* data);
void        vkh_copy_with_staging_buf_implicit(VKHinstance* instance, VkBuffer buf, uint64_t size, uint64_t offset, void* data);

void        vkh_transition_image_layout       (VKHinstance* instance, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

//----------------------------------------------------------------------------//

uint32_t* vkh_load_spirv(const char* path, uint64_t* size);
void      vkh_free_spirv(uint32_t* code);

VkShaderModule vkh_create_shader_module (VKHinstance* instance, uint64_t codeSize, uint32_t* code);
void           vkh_destroy_shader_module(VKHinstance* instance, VkShaderModule module);

//----------------------------------------------------------------------------//

VkCommandBuffer vkh_start_single_time_command(VKHinstance* inst);
void            vkh_end_single_time_command  (VKHinstance* inst, VkCommandBuffer commandBuffer);

//----------------------------------------------------------------------------//

//NOTE: only supports 1 desciptor layout, FIXME
//NOTE: only supports vert/frag shaders, FIXME
VKHgraphicsPipeline* vkh_pipeline_create    ();
void                 vkh_pipeline_destroy   (VKHgraphicsPipeline* pipeline);

vkh_bool_t           vkh_pipeline_generate  (VKHgraphicsPipeline* pipeline, VKHinstance* instance, VkRenderPass renderPass, uint32_t subpass);
void                 vkh_pipeline_cleanup   (VKHgraphicsPipeline* pipeline, VKHinstance* instance);

void vkh_pipeline_add_desc_set_binding      (VKHgraphicsPipeline* pipeline, VkDescriptorSetLayoutBinding binding);
void vkh_pipeline_add_dynamic_state         (VKHgraphicsPipeline* pipeline, VkDynamicState state);
void vkh_pipeline_add_vertex_input_binding  (VKHgraphicsPipeline* pipeline, VkVertexInputBindingDescription binding);
void vkh_pipeline_add_vertex_input_attrib   (VKHgraphicsPipeline* pipeline, VkVertexInputAttributeDescription attrib);
void vkh_pipeline_add_viewport              (VKHgraphicsPipeline* pipeline, VkViewport viewport);
void vkh_pipeline_add_scissor               (VKHgraphicsPipeline* pipeline, VkRect2D scissor);
void vkh_pipeline_add_color_blend_attachment(VKHgraphicsPipeline* pipeline, VkPipelineColorBlendAttachmentState attachment);
void vkh_pipeline_add_push_constant         (VKHgraphicsPipeline* pipeline, VkPushConstantRange pushConstant);

void vkh_pipeline_set_vert_shader           (VKHgraphicsPipeline* pipeline, VkShaderModule shader);
void vkh_pipeline_set_frag_shader           (VKHgraphicsPipeline* pipeline, VkShaderModule shader);

void vkh_pipeline_set_input_assembly_state  (VKHgraphicsPipeline* pipeline, VkPrimitiveTopology topology, VkBool32 primitiveRestart);
void vkh_pipeline_set_tesselation_state     (VKHgraphicsPipeline* pipeline, uint32_t patchControlPoints);
void vkh_pipeline_set_raster_state          (VKHgraphicsPipeline* pipeline, VkBool32 depthClamp, VkBool32 rasterDiscard, VkPolygonMode polyMode,
                                             VkCullModeFlags cullMode, VkFrontFace frontFace, VkBool32 depthBias, float biasConstFactor, float biasClamp, float biasSlopeFactor);
void vkh_pipeline_set_multisample_state     (VKHgraphicsPipeline* pipeline, VkSampleCountFlagBits rasterSamples, VkBool32 sampleShading,
                                             float minSampleShading, const VkSampleMask* sampleMask, VkBool32 alphaToCoverage, VkBool32 alphaToOne);
void vkh_pipeline_set_depth_stencil_state   (VKHgraphicsPipeline* pipeline, VkBool32 depthTest, VkBool32 depthWrite, VkCompareOp depthCompareOp,
                                             VkBool32 depthBoundsTest, VkBool32 stencilTest, VkStencilOpState front, VkStencilOpState back, float minDepthBound, float maxDepthBound);
void vkh_pipeline_set_color_blend_state     (VKHgraphicsPipeline* pipeline, VkBool32 logicOpEnable, VkLogicOp logicOp, float rBlendConstant,
                                             float gBlendConstant, float bBlendConstant, float aBlendConstant);

//----------------------------------------------------------------------------//

VKHcomputePipeline* vkh_compute_pipeline_create              ();
void                vkh_compute_pipeline_destroy             (VKHcomputePipeline* pipeline);

vkh_bool_t          vkh_compute_pipeline_generate            (VKHcomputePipeline* pipeline, VKHinstance* instance);
void                vkh_compute_pipeline_cleanup             (VKHcomputePipeline* pipeline, VKHinstance* instance);

void                vkh_compute_pipeline_add_desc_set_binding(VKHcomputePipeline* pipeline, VkDescriptorSetLayoutBinding binding);
void                vkh_compute_pipeline_add_push_constant   (VKHcomputePipeline* pipeline, VkPushConstantRange pushConstant);

void                vkh_compute_pipeline_set_shader          (VKHcomputePipeline* pipeline, VkShaderModule shader);

//----------------------------------------------------------------------------//

VKHdescriptorSets* vkh_descriptor_sets_create         (uint32_t count);
void               vkh_descriptor_sets_destroy        (VKHdescriptorSets* descriptorSets);

vkh_bool_t         vkh_desctiptor_sets_generate       (VKHdescriptorSets* descriptorSets, VKHinstance* instance, VkDescriptorSetLayout layout);
void               vkh_descriptor_sets_cleanup        (VKHdescriptorSets* descriptorSets, VKHinstance* instance);

void               vkh_descriptor_sets_add_buffers    (VKHdescriptorSets* descriptorSets, uint32_t index, VkDescriptorType type, uint32_t binding, uint32_t arrayElem, 
                                                       uint32_t count, VkDescriptorBufferInfo* bufferInfos);
void               vkh_descriptor_sets_add_images     (VKHdescriptorSets* descriptorSets, uint32_t index, VkDescriptorType type, uint32_t binding, uint32_t arrayElem, 
                                                       uint32_t count, VkDescriptorImageInfo* imageInfos);
void               vkh_descriptor_sets_add_texel_views(VKHdescriptorSets* descriptorSets, uint32_t index, VkDescriptorType type, uint32_t binding, uint32_t arrayElem, 
                                                       uint32_t count, VkBufferView* texelViews);

//----------------------------------------------------------------------------//

#ifdef __cplusplus
} //extern "C"
#endif

#endif //#ifndef VKH_H