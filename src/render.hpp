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

	uint32 graphicsFamilyIdx;
	uint32 presentFamilyIdx;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapchain;
	VkFormat swapchainFormat;
	VkExtent2D swapchainExtent;
	uint32 swapchainImageCount;
	VkImage* swapchainImages;
	VkImageView* swapchainImageViews;

	#if RENDER_VALIDATION_LAYERS
		VkDebugUtilsMessengerEXT debugMessenger;
	#endif
};

//----------------------------------------------------------------------------//

bool render_init(RenderInstance** instance, uint32 windowW, uint32 windowH, const char* windowName);
void render_quit(RenderInstance* instance);

void render_resize_swapchain(RenderInstance* instance, uint32 w, uint32 h);

VkImageView render_create_image_view(RenderInstance* instance, VkImage image, VkFormat format, VkImageAspectFlags aspects, uint32 mipLevels);
void render_destroy_image_view(RenderInstance* instance, VkImageView view);

#endif