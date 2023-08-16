#include "render.hpp"
#include <stdio.h>
#include <malloc.h>
#include <string.h>

//----------------------------------------------------------------------------//

static bool _render_init_glfw(RenderInstance* instance, uint32 w, uint32 h, const char* name);
static void _render_quit_glfw(RenderInstance* instance);

static bool _render_create_vk_instance(RenderInstance* instance, const char* name);
static void _render_destroy_vk_instance(RenderInstance* instance);

static bool _render_pick_physical_device(RenderInstance* instance);

static bool _render_create_device(RenderInstance* instance);
static void _render_destroy_vk_device(RenderInstance* instance);

static bool _render_create_swapchain(RenderInstance* instance, uint32 w, uint32 h);
static void _render_destroy_swapchain(RenderInstance* instance);

//----------------------------------------------------------------------------//

static VKAPI_ATTR VkBool32 _render_vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT sevrerity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
	void* userData
);

//----------------------------------------------------------------------------//

static void _render_message_log(const char* message, const char* file, int32 line);
#define MSG_LOG(m) _render_message_log(m, __FILE__, __LINE__)

static void _render_error_log(const char* message, const char* file, int32 line);
#define ERROR_LOG(m) _render_error_log(m, __FILE__, __LINE__)

//----------------------------------------------------------------------------//

#if RENDER_VALIDATION_LAYERS
	#define REQUIRED_LAYER_COUNT 1
	const char* REQUIRED_LAYERS[REQUIRED_LAYER_COUNT] = {"VK_LAYER_KHRONOS_validation"};
#endif

#define REQUIRED_DEVICE_EXTENSION_COUNT 1
const char* REQUIRED_DEVICE_EXTENSIONS[REQUIRED_DEVICE_EXTENSION_COUNT] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

//----------------------------------------------------------------------------//

bool render_init(RenderInstance** instance, uint32 windowW, uint32 windowH, const char* windowName)
{
	*instance = new RenderInstance;
	RenderInstance* inst = *instance;

	if(!_render_init_glfw(inst, windowW, windowH, windowName))
		return false;

	if(!_render_create_vk_instance(inst, windowName))
		return false;

	if(!_render_pick_physical_device(inst))
		return false;

	if(!_render_create_device(inst))
		return false;

	if(!_render_create_swapchain(inst, windowW, windowH))
		return false;

	return true;
}

void render_quit(RenderInstance* inst)
{
	_render_destroy_swapchain(inst);
	_render_destroy_vk_device(inst);
	_render_destroy_vk_instance(inst);
	_render_quit_glfw(inst);

	delete inst;
}

//----------------------------------------------------------------------------//

void render_resize_swapchain(RenderInstance* inst, uint32 w, uint32 h)
{
	if(w == 0 || h == 0) //TODO: test
		return;
	
	vkDeviceWaitIdle(inst->device);

	_render_destroy_swapchain(inst);
	_render_create_swapchain(inst, w, h);
}

//----------------------------------------------------------------------------//

VkImageView render_create_image_view(RenderInstance* inst, VkImage image, VkFormat format, VkImageAspectFlags aspects, uint32 mipLevels)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspects;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView view;
	if(vkCreateImageView(inst->device, &viewInfo, NULL, &view) != VK_SUCCESS)
		ERROR_LOG("failed to create image view");

	return view;
}

void render_destroy_image_view(RenderInstance* inst, VkImageView view)
{
	vkDestroyImageView(inst->device, view, NULL);
}

//----------------------------------------------------------------------------//

static bool _render_init_glfw(RenderInstance* inst, uint32 w, uint32 h, const char* name)
{
	MSG_LOG("initlalizing GLFW...");

	if(!glfwInit())
	{
		ERROR_LOG("failed to initialize GLFW");
		return false;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	inst->window = glfwCreateWindow(w, h, name, NULL, NULL);
	if(!inst->window)
	{
		ERROR_LOG("failed to create GLFW window");
		return false;
	}

	return true;
}

static void _render_quit_glfw(RenderInstance* inst)
{
	MSG_LOG("quitting GLFW...");

	glfwDestroyWindow(inst->window);
	glfwTerminate();
}

static bool _render_create_vk_instance(RenderInstance* inst, const char* name)
{
	MSG_LOG("creating Vulkan instance...");

	//get required GLFW extensions:
	//---------------
	uint32 requiredExtensionCount;
	const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
	if(!requiredExtensions)
	{
		ERROR_LOG("Vulkan rendering not supported on this machine");
		return false;
	}
	bool freeExtensionList = false;

	#if RENDER_VALIDATION_LAYERS
	{
		const char** requiredExtensionsValidation = (const char**)malloc((requiredExtensionCount + 1) * sizeof(const char*));
		memcpy(requiredExtensionsValidation, requiredExtensions, requiredExtensionCount * sizeof(const char*));
		requiredExtensionsValidation[requiredExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

		requiredExtensionCount++;
		requiredExtensions = requiredExtensionsValidation;
		freeExtensionList = true;
	}
	#endif

	//check if glfw extensions are supported:
	//---------------
	uint32 extensionCount;
	vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
	VkExtensionProperties* extensions = (VkExtensionProperties*)malloc(extensionCount * sizeof(VkExtensionProperties));
	vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);

	for(int32 i = 0; i < requiredExtensionCount; i++)
	{
		bool found = false;
		for(int32 j = 0; j < extensionCount; j++)
			if(strcmp(requiredExtensions[i], extensions[j].extensionName) == 0)
			{
				found = true;
				break;
			}
		
		if(!found)
		{
			ERROR_LOG("1 or more required GLFW extensions not supported");;
			return false;
		}
	}

	free(extensions);

	//check if validation layers are supported:
	//---------------
	uint32 requiredLayerCount = 0;
	const char** requiredLayers;
	
	#if RENDER_VALIDATION_LAYERS
	{
		requiredLayerCount = REQUIRED_LAYER_COUNT;
		requiredLayers = REQUIRED_LAYERS;

		uint32 supportedLayerCount;
		vkEnumerateInstanceLayerProperties(&supportedLayerCount, NULL);
		VkLayerProperties* supportedLayers = (VkLayerProperties*)malloc(supportedLayerCount * sizeof(VkLayerProperties));
		vkEnumerateInstanceLayerProperties(&supportedLayerCount, supportedLayers);

		bool found = false;
		for(int32 i = 0; i < requiredLayerCount; i++)
		{
			bool found = false;
			for(int32 j = 0; j < supportedLayerCount; j++)
				if(strcmp(requiredLayers[i], supportedLayers[j].layerName) == 0)
				{
					found = true;
					break;
				}

			if(!found)
			{
				ERROR_LOG("1 or more required validation layers not supported");
				return false;
			}
		}

		free(supportedLayers);
	}
	#endif

	//create instance creation info structs:
	//---------------
	VkApplicationInfo appInfo = {}; //most of this stuff is pretty useless, just for drivers to optimize certain programs
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = name;
	appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	appInfo.pEngineName = "";
	appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	
	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledExtensionCount = requiredExtensionCount;
	instanceInfo.ppEnabledExtensionNames = requiredExtensions;
	instanceInfo.enabledLayerCount = requiredLayerCount;
	instanceInfo.ppEnabledLayerNames = requiredLayers;

	#if RENDER_VALIDATION_LAYERS //not in blocks as debugInfo is used later
		VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
		debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugInfo.messageSeverity = 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		debugInfo.messageType = 
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		debugInfo.pfnUserCallback = _render_vk_debug_callback;
		debugInfo.pUserData = inst;

		instanceInfo.pNext = &debugInfo;
	#endif

	if(vkCreateInstance(&instanceInfo, NULL, &inst->instance) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create Vulkan instance");
		return false;
	}

	if(freeExtensionList)
		free(requiredExtensions);
	
	//create debug messenger:
	//---------------
	#if RENDER_VALIDATION_LAYERS
	{
		PFN_vkCreateDebugUtilsMessengerEXT createDebugMessenger = 
			(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(inst->instance, "vkCreateDebugUtilsMessengerEXT");
		
		if(!createDebugMessenger)
		{
			ERROR_LOG("could not find function \"vkCreateDebugUtilsMessengerEXT\"");
			return false;
		}

		if(createDebugMessenger(inst->instance, &debugInfo, NULL, &inst->debugMessenger) != VK_SUCCESS)
		{
			ERROR_LOG("failed to create debug messenger");
			return false;
		}
	}	
	#endif

	//create surface:
	//---------------
	if(glfwCreateWindowSurface(inst->instance, inst->window, NULL, &inst->surface) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create window surface");
		return false;
	}

	return true;
}

static void _render_destroy_vk_instance(RenderInstance* inst)
{
	MSG_LOG("destroying Vulkan instance...");

	vkDestroySurfaceKHR(inst->instance, inst->surface, NULL);

	#if RENDER_VALIDATION_LAYERS
	{
		PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessenger =
			(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(inst->instance, "vkDestroyDebugUtilsMessengerEXT");
		
		if(destroyDebugMessenger)
			destroyDebugMessenger(inst->instance, inst->debugMessenger, NULL);
		else
			ERROR_LOG("could not find function \"vkDestroyDebugUtilsMessengerEXT\"");
	}
	#endif
	
	vkDestroyInstance(inst->instance, NULL);
}

static bool _render_pick_physical_device(RenderInstance* inst)
{
	MSG_LOG("picking physical device...");

	uint32 deviceCount;
	vkEnumeratePhysicalDevices(inst->instance, &deviceCount, NULL);

	if(deviceCount == 0)
	{
		ERROR_LOG("failed to find a physical device that supports Vulkan");
		return false;
	}

	VkPhysicalDevice* devices = (VkPhysicalDevice*)malloc(deviceCount * sizeof(VkPhysicalDevice));
	vkEnumeratePhysicalDevices(inst->instance, &deviceCount, devices);

	int32 maxScore = -1;
	for(int32 i = 0; i < deviceCount; i++)
	{
		int32 score = 0;

		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceProperties      (devices[i], &properties);
		vkGetPhysicalDeviceFeatures        (devices[i], &features);
		vkGetPhysicalDeviceMemoryProperties(devices[i], &memProperties);

		//check if required queue families are supported:
		//---------------
		int32 graphicsFamilyIdx = -1;
		int32 presentFamilyIdx = -1;

		uint32 queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, NULL);
		VkQueueFamilyProperties* queueFamilies = (VkQueueFamilyProperties*)malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, queueFamilies);

		for(int32 j = 0; j < queueFamilyCount; j++)
		{
			if(queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) //TODO: see if there is a way to determine most optimal queue families
				graphicsFamilyIdx = j;
			
			VkBool32 presentSupport;
			vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, inst->surface, &presentSupport);
			if(presentSupport)
				presentFamilyIdx = j;

			if(graphicsFamilyIdx > 0 && presentFamilyIdx > 0)
				break;
		}

		free(queueFamilies);

		if(graphicsFamilyIdx < 0 || presentFamilyIdx < 0)
			continue;

		//check if required extensions are supported:
		//---------------
		bool extensionsSupported = true;

		uint32 extensionCount;
		vkEnumerateDeviceExtensionProperties(devices[i], NULL, &extensionCount, NULL);
		VkExtensionProperties* extensions = (VkExtensionProperties*)malloc(extensionCount * sizeof(VkExtensionProperties));
		vkEnumerateDeviceExtensionProperties(devices[i], NULL, &extensionCount, extensions);

		for(int32 j = 0; j < REQUIRED_DEVICE_EXTENSION_COUNT; j++)
		{
			bool found = false;
			for(int32 k = 0; k < extensionCount; k++)
				if(strcmp(REQUIRED_DEVICE_EXTENSIONS[j], extensions[k].extensionName) == 0)
				{
					found = true;
					break;
				}

			if(!found)
			{
				extensionsSupported = false;
				break;
			}
		}

		free(extensions);

		if(!extensionsSupported)
			continue;

		//check if swapchain is supported:
		//---------------
		uint32 swapchainFormatCount, swapchainPresentModeCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR     (devices[i], inst->surface, &swapchainFormatCount     , NULL);
		vkGetPhysicalDeviceSurfacePresentModesKHR(devices[i], inst->surface, &swapchainPresentModeCount, NULL);

		if(swapchainFormatCount == 0 || swapchainPresentModeCount == 0)
			continue;

		//score device:
		//---------------

		//TODO: consider more features/properties of the device, currently we only consider whether or not it is discrete
		if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			score += 1000;

		if(score > maxScore)
		{
			inst->physicalDevice = devices[i];
			inst->graphicsFamilyIdx = graphicsFamilyIdx;
			inst->presentFamilyIdx = presentFamilyIdx;

			maxScore = score;
		}
	}

	if(maxScore < 0)
	{
		ERROR_LOG("failed to find a suitable physical device");
		return false;
	}

	free(devices);

	return true;
}

static bool _render_create_device(RenderInstance* inst)
{
	MSG_LOG("creating Vulkan device...");

	//create queue infos:
	//---------------

	//TODO: extend this to work with more than just 2 queue types
	//TODO: allow user to define which queues they would like, instead of just getting graphics and present
	uint32 queueCount = 0;
	uint32 queueIndices[2];
	if(inst->graphicsFamilyIdx == inst->presentFamilyIdx)
	{
		queueCount = 1;
		queueIndices[0] = inst->graphicsFamilyIdx;
	}
	else
	{
		queueCount = 2;
		queueIndices[0] = inst->graphicsFamilyIdx;
		queueIndices[1] = inst->presentFamilyIdx;
	}

	VkDeviceQueueCreateInfo queueInfos[2];
	for(int32 i = 0; i < queueCount; i++)
	{
		f32 priotity = 1.0f;

		VkDeviceQueueCreateInfo queueInfo = {};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = queueIndices[i]; //TODO: see how we can optimize this, when would we want multiple queues?
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &priotity;

		queueInfos[i] = queueInfo;
	}

	//set features:
	//---------------
	VkPhysicalDeviceFeatures features = {}; //TODO: allow wanted features to be passed in

	//create device:
	//---------------
	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.queueCreateInfoCount = queueCount;
	deviceInfo.pQueueCreateInfos = queueInfos;
	deviceInfo.pEnabledFeatures = &features;
	deviceInfo.enabledExtensionCount = REQUIRED_DEVICE_EXTENSION_COUNT;
	deviceInfo.ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS;
	#if RENDER_VALIDATION_LAYERS
	{
		deviceInfo.enabledLayerCount = REQUIRED_LAYER_COUNT;
		deviceInfo.ppEnabledLayerNames = REQUIRED_LAYERS;
	}
	#endif

	if(vkCreateDevice(inst->physicalDevice, &deviceInfo, NULL, &inst->device) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create Vulkan device");
		return false;
	}

	vkGetDeviceQueue(inst->device, inst->graphicsFamilyIdx, 0, &inst->graphicsQueue);
	vkGetDeviceQueue(inst->device, inst->presentFamilyIdx, 0, &inst->presentQueue);

	return true;
}

static void _render_destroy_vk_device(RenderInstance* inst)
{
	MSG_LOG("destroying Vulkan device...");

	vkDestroyDevice(inst->device, NULL);
}

static bool _render_create_swapchain(RenderInstance* inst, uint32 w, uint32 h)
{
	MSG_LOG("creating Vulkan swapchain...");

	//get format and present mode:
	//---------------
	uint32 formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(inst->physicalDevice, inst->surface, &formatCount, NULL);
	VkSurfaceFormatKHR* supportedFormats = (VkSurfaceFormatKHR*)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
	vkGetPhysicalDeviceSurfaceFormatsKHR(inst->physicalDevice, inst->surface, &formatCount, supportedFormats);

	VkSurfaceFormatKHR format = supportedFormats[0];
	for(int32 i = 0; i < formatCount; i++)
		if(supportedFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && supportedFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			format = supportedFormats[i];
			break;
		}

	free(supportedFormats);

	uint32 presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(inst->physicalDevice, inst->surface, &presentModeCount, NULL);
	VkPresentModeKHR* supportedPresentModes = (VkPresentModeKHR*)malloc(presentModeCount * sizeof(VkPresentModeKHR));
	vkGetPhysicalDeviceSurfacePresentModesKHR(inst->physicalDevice, inst->surface, &presentModeCount, supportedPresentModes);
	
	VkPresentModeKHR presentMode = supportedPresentModes[0];
	for(int32 i = 0; i < presentModeCount; i++)
		if(supportedPresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			presentMode = supportedPresentModes[i];

	free(supportedPresentModes);

	//get extent:
	//---------------
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(inst->physicalDevice, inst->surface, &capabilities);

	VkExtent2D extent;
	if(capabilities.currentExtent.width != UINT32_MAX)
	{
		//TODO: does this work when the window is resized?
		extent = capabilities.currentExtent; //window already defined size for us
	}
	else
	{
		extent = {w, h};
		
		//clamping:
		extent.width = extent.width > capabilities.maxImageExtent.width ? capabilities.maxImageExtent.width : extent.width;
		extent.width = extent.width < capabilities.minImageExtent.width ? capabilities.minImageExtent.width : extent.width;

		extent.height = extent.height > capabilities.maxImageExtent.height ? capabilities.maxImageExtent.height : extent.height;
		extent.height = extent.height < capabilities.minImageExtent.height ? capabilities.minImageExtent.height : extent.height;
	}

	//get image count:
	//---------------
	uint32 imageCount = capabilities.minImageCount + 1;
	if(capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
		imageCount = capabilities.maxImageCount;
	
	//get swapchain:
	//---------------
	VkSwapchainCreateInfoKHR swapchainInfo = {};
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.surface = inst->surface;
	swapchainInfo.minImageCount = imageCount;
	swapchainInfo.imageFormat = format.format;
	swapchainInfo.imageColorSpace = format.colorSpace;
	swapchainInfo.imageExtent = extent;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInfo.preTransform = capabilities.currentTransform;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.presentMode = presentMode;
	swapchainInfo.clipped = VK_TRUE;
	swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

	if(inst->graphicsFamilyIdx != inst->presentFamilyIdx)
	{
		uint32 indices[] = {inst->graphicsFamilyIdx, inst->presentFamilyIdx};
	
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainInfo.queueFamilyIndexCount = 2;
		swapchainInfo.pQueueFamilyIndices = indices;
	}
	else
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if(vkCreateSwapchainKHR(inst->device, &swapchainInfo, NULL, &inst->swapchain) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create Vulkan swapchain");
		return false;
	}

	inst->swapchainExtent = extent;
	inst->swapchainFormat = format.format;

	vkGetSwapchainImagesKHR(inst->device, inst->swapchain, &inst->swapchainImageCount, NULL);
	inst->swapchainImages     =     (VkImage*)malloc(inst->swapchainImageCount * sizeof(VkImage));
	inst->swapchainImageViews = (VkImageView*)malloc(inst->swapchainImageCount * sizeof(VkImageView));
	vkGetSwapchainImagesKHR(inst->device, inst->swapchain, &inst->swapchainImageCount, inst->swapchainImages);

	for(int32 i = 0; i < inst->swapchainImageCount; i++)
		inst->swapchainImageViews[i] = render_create_image_view(inst, inst->swapchainImages[i], inst->swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	return true;
}

static void _render_destroy_swapchain(RenderInstance* inst)
{
	MSG_LOG("destroying Vulkan swapchain...");

	for(int32 i = 0; i < inst->swapchainImageCount; i++)
		render_destroy_image_view(inst, inst->swapchainImageViews[i]);
	
	free(inst->swapchainImages);
	free(inst->swapchainImageViews);

	vkDestroySwapchainKHR(inst->device, inst->swapchain, NULL);
}

//----------------------------------------------------------------------------//

static VKAPI_ATTR VkBool32 _render_vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT sevrerity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
	void* userData)
{
	if(sevrerity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		return VK_FALSE;

	printf("RENDER VALIDATION LAYER - %s\n\n", callbackData->pMessage);
	return VK_FALSE;
}

//----------------------------------------------------------------------------//

static void _render_message_log(const char* message, const char* file, int32 line)
{
	printf("RENDER MESSAGE in %s at line %i - \"%s\"\n\n", file, line, message);
}

static void _render_error_log(const char* message, const char* file, int32 line)
{
	printf("RENDER ERROR in %s at line %i - \"%s\"\n\n", file, line, message);
}