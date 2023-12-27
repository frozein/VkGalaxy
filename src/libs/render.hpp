#ifndef RENDER_H
#define RENDER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "globals.hpp"

#define RENDER_VALIDATION_LAYERS 1

//----------------------------------------------------------------------------//

struct RenderInstance
{
	GLFWwindow* window;

	VkInstance instance;
	VkDevice device;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;

	uint32 graphicsComputeFamilyIdx;
	uint32 presentFamilyIdx;
	VkQueue graphicsQueue;
	VkQueue computeQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapchain;
	VkFormat swapchainFormat;
	VkExtent2D swapchainExtent;
	uint32 swapchainImageCount;
	VkImage* swapchainImages;
	VkImageView* swapchainImageViews;

	VkCommandPool commandPool;

	#if RENDER_VALIDATION_LAYERS
		VkDebugUtilsMessengerEXT debugMessenger;
	#endif
};

//----------------------------------------------------------------------------//

bool render_init(RenderInstance** instance, uint32 windowW, uint32 windowH, const char* windowName);
void render_quit(RenderInstance* instance);

void render_resize_swapchain(RenderInstance* instance, uint32 w, uint32 h);

VkImage render_create_image(RenderInstance* instance, uint32 w, uint32 h, uint32 mipLevels, VkSampleCountFlagBits samples, 
	VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceMemory* memory);
void render_destroy_image(RenderInstance* instance, VkImage image, VkDeviceMemory memory);

VkImageView render_create_image_view(RenderInstance* instance, VkImage image, VkFormat format, VkImageAspectFlags aspects, uint32 mipLevels);
void render_destroy_image_view(RenderInstance* instance, VkImageView view);

VkBuffer render_create_buffer(RenderInstance* instance, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceMemory* memory);
void render_destroy_buffer(RenderInstance* instance, VkBuffer buffer, VkDeviceMemory memory);

void render_copy_buffer(RenderInstance* instance, VkBuffer src, VkBuffer dst, VkDeviceSize size, uint64 srcOffset, uint64 dstOffset);
void render_copy_buffer_to_image(RenderInstance* instance, VkBuffer buffer, VkImage image, uint32 width, uint32 height);

void render_upload_with_staging_buffer(RenderInstance* instance, VkBuffer buf, uint64 size, uint64 offset, void* data);
void render_upload_with_staging_buffer(RenderInstance* instance, VkBuffer stagingBuf, VkDeviceMemory stagingBufMem, VkBuffer buf, uint64 size, uint64 offset, void* data);

void render_transition_image_layout(RenderInstance* instance, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

uint32* render_load_spirv(const char* path, uint64* size);
void render_free_spirv(uint32* code);

VkShaderModule render_create_shader_module(RenderInstance* instance, uint64 codeSize, uint32* code);
void render_destroy_shader_module(RenderInstance* instance, VkShaderModule module);

#endif