#ifndef VKH_H
#define VKH_H

#ifdef __cplusplus
extern "C" 
{
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdint.h>

#define VKH_VALIDATION_LAYERS 1

//----------------------------------------------------------------------------//

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

typedef int32_t vkh_bool_t;
#define VKH_TRUE  1
#define VKH_FALSE 0

//----------------------------------------------------------------------------//

vkh_bool_t vkh_init(VKHinstance** instance, uint32_t windowW, uint32_t windowH, const char* windowName);
void vkh_quit(VKHinstance* instance);

void vkh_resize_swapchain(VKHinstance* instance, uint32_t w, uint32_t h);

VkImage vkh_create_image(VKHinstance* instance, uint32_t w, uint32_t h, uint32_t mipLevels, VkSampleCountFlagBits samples, 
	VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceMemory* memory);
void vkh_destroy_image(VKHinstance* instance, VkImage image, VkDeviceMemory memory);

VkImageView vkh_create_image_view(VKHinstance* instance, VkImage image, VkFormat format, VkImageAspectFlags aspects, uint32_t mipLevels);
void vkh_destroy_image_view(VKHinstance* instance, VkImageView view);

VkBuffer vkh_create_buffer(VKHinstance* instance, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceMemory* memory);
void vkh_destroy_buffer(VKHinstance* instance, VkBuffer buffer, VkDeviceMemory memory);

void vkh_copy_buffer(VKHinstance* instance, VkBuffer src, VkBuffer dst, VkDeviceSize size, uint64_t srcOffset, uint64_t dstOffset);
void vkh_copy_buffer_to_image(VKHinstance* instance, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

void vkh_upload_with_staging_buffer(VKHinstance* instance, VkBuffer stagingBuf, VkDeviceMemory stagingBufMem, VkBuffer buf, uint64_t size, uint64_t offset, void* data);
void vkh_upload_with_staging_buffer_implicit(VKHinstance* instance, VkBuffer buf, uint64_t size, uint64_t offset, void* data);

void vkh_transition_image_layout(VKHinstance* instance, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

uint32_t* vkh_load_spirv(const char* path, uint64_t* size);
void vkh_free_spirv(uint32_t* code);

VkShaderModule vkh_create_shader_module(VKHinstance* instance, uint64_t codeSize, uint32_t* code);
void vkh_destroy_shader_module(VKHinstance* instance, VkShaderModule module);

#ifdef __cplusplus
} //extern "C"
#endif

#endif //#ifndef VKH_H