#pragma once

#pragma warning(push)
#pragma warning(disable : 26812) 

#define GLFW_INCLUDE_VULKAN
#define GLFW_DLL
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <vulkan/vulkan.h>
#include <vector>
//#include <vulkan/vulkan.hpp>
#include <Windows.h>
#include <iostream>
#include <cstring>
#include <map> //C++17
#include <optional> //C++17 


#define TEST_FAILD 0

enum state{GLFW_TEST,VULKAN_LOADING,VALIDATION_LAYERS};

struct QueueFamiliesIndices
{
	std::optional<uint32_t> GraphicsFamily; //put to be optional type to distinguish between has value or not,
	//as to determine a value in uint32_t to be a non-value index, theorically might be a real value of the returned queue
	bool isComplete() {
		return GraphicsFamily.has_value();
	}
};


class VRender
{
public:

	bool VulkanLoadingStatus[3];
	VRender();
	~VRender();

	//callbacks
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {

		//if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		std::cerr << "Validation layer Error: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE; //need to keep it false as it will abort the call that triggered the callback with VK_ERROR_VALIDATION_FAILED_EXT error if it is true, we use it to test validation only
	}


private:

	//extensions-based functions:
	VkResult CreateDebugUtilsMessengerEXT(const VkInstance& instance,
		VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	void DestroyDebugUtilsMessengerEXT(const VkInstance& instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);

	//validation functions:
	bool CheckValidationLayerSupport();
	bool ValidationState();
	void SetupDebugMessenger();

	//extension's functions:
	bool CheckExtensionsBeforeInstance();
	std::vector<const char*> GLFWGetRequiredExtension();

	//devices functions
	void PickPhysicalDevice(VkQueueFlagBits bit);
	bool isDeviceSuitable(VkPhysicalDevice device, VkQueueFlagBits bit);
	int RateDeviceSuitability(VkPhysicalDevice device, VkQueueFlagBits bit);

	//Queues
	std::vector<VkQueueFamilyProperties> FindQueueFamilies(VkPhysicalDevice device);
	QueueFamiliesIndices CheckQueueFamily(VkPhysicalDevice device, VkQueueFlagBits bit);

	//first steps function
	bool GLFWsetter();
	bool Initiliazer();
	void CreateInstance();


	//data
	VkInstance VK_Instance;
	GLFWwindow* VK_Window;
	enum class DEVICE_PICKING_UP_PATTERN{USE_FIRST_SUITABLE_DEVICE,USE_BEST_RATED_SUITABLE_DEVICE};
	DEVICE_PICKING_UP_PATTERN pattern;


	//validation layers debugger messenger
	VkDebugUtilsMessengerEXT debugMessenger;
	VkDebugUtilsMessengerCreateInfoEXT VK_Messenger_CreateInfo{};

	//devices
	VkPhysicalDevice PhysicalDevice;
	std::vector<VkPhysicalDevice> VK_Devices;
	VkPhysicalDeviceProperties VK_Device_Properties;
	VkPhysicalDeviceFeatures VK_Device_Features;
	std::multimap<int, VkPhysicalDevice> rated_devices_candidates;

	//Queues
	std::vector<VkQueueFamilyProperties> VK_Device_QueueFamilies;

	//Structs
	VkApplicationInfo VK_AppInfo{};
	VkInstanceCreateInfo VK_CreateInfo{};

	//Extensions
	std::vector<VkExtensionProperties> VK_Available_Extensions;
	std::vector<const char*> VK_Extensions;

	//validation layers
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif
	uint32_t LayersCount = 0;
	std::vector<const char*> VK_ValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
	std::vector<VkLayerProperties> VK_AvailableValidationLayers;



public:

	void Render();

	std::string GetErrorName(size_t index);

	void PrintGLFWExtensions(std::vector<const char*> vec);

};

