#pragma once

#pragma warning(push)
#pragma warning(disable : 26812) 

#define NOMINMAX //avoid windows vc++ defined min/max funcs

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
//#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#define GLFW_DLL
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <Windows.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <map> //C++17
#include <optional> //C++17 
#include <set>
#include <cstdint> //for UINT32_MAX
#include <algorithm>
#include <fstream>

namespace Vulkan_Engine {

#define TEST_FAILD 0

enum state{GLFW_TEST,VULKAN_LOADING,VALIDATION_LAYERS};

struct QueueFamiliesIndices
{
	//put to be optional type to distinguish between has value or not,
	//as to determine a value in uint32_t to be a non-value index, theorically might be a real value of the returned queue
	std::optional<uint32_t> GraphicsFamily; 
	std::optional<uint32_t> PresentFamily;
	bool isComplete() {
		return GraphicsFamily.has_value() && PresentFamily.has_value();
	}
};

struct SwapChainSupportDetails 
{
	VkSurfaceCapabilitiesKHR SurfaceCapabilities{};
	std::vector<VkSurfaceFormatKHR> SurfaceFormats;
	std::vector<VkPresentModeKHR> SurfacePresentMode;
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

		//vulkan extension's functions:
		bool CheckExtensionsBeforeInstance();
		std::vector<const char*> GLFWGetRequiredExtension();

		//device extension's functions
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

		//Physical devices functions
		void PickPhysicalDevice(VkQueueFlagBits bit);
		bool isDeviceSuitable(VkPhysicalDevice device, VkQueueFlagBits bit);
		int RateDeviceSuitability(VkPhysicalDevice device, VkQueueFlagBits bit);

		//Queues
		std::vector<VkQueueFamilyProperties> FindQueueFamilies(VkPhysicalDevice device);
		QueueFamiliesIndices CheckForQueueFamily(VkPhysicalDevice device, VkQueueFlagBits bit, bool CheckForPresentQueue);

		//Logical devices functions
		void CreateLogicalDevice();

		//surface functions
		void CreateSurface();

		//SwapChain
		bool QuerySwapChainSupport(VkPhysicalDevice device);
		VkSurfaceFormatKHR SelectSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR  SelectSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availableModes);
		VkExtent2D SelectSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		void CreateSwapChain();

		//SwapChain ImageViews
		void CreateImageView();

		//Shaders Operations
		bool LoadShaderSource(const char* path, std::string& src, int majorVersion, int minorVersion);
		bool LoadShaderSource(const char* path, std::vector<char>& src);
		bool LoadShaderSource(const char* path, std::vector<char>& src, int majorVersion, int minorVersion);
		void LoadCompileShaders();
		void PrintShadersMap();

		//Shaders Modules
		VkShaderModule CreateShaderModule(const char* ShaderName, const std::vector<char>& code);

		//Render Passes
		void CreateRenderPass();

		//Graphics Pipline
		void CreateGraphicsPipeline();

		//first steps functions
		bool GLFWsetter();
		bool Initiliazer();
		void CreateInstance();

		//data
		VkInstance VK_Instance;
		GLFWwindow* VK_Window;
		enum class DEVICE_PICKING_UP_PATTERN { USE_FIRST_SUITABLE_DEVICE, USE_BEST_RATED_SUITABLE_DEVICE };
		DEVICE_PICKING_UP_PATTERN pattern;


		//validation layers debugger messenger
		VkDebugUtilsMessengerEXT debugMessenger;
		VkDebugUtilsMessengerCreateInfoEXT VK_Messenger_CreateInfo;


		//physical devices
		VkPhysicalDevice PhysicalDevice;
		std::vector<VkPhysicalDevice> VK_Phy_Devices;
		VkPhysicalDeviceProperties VK_Phy_Device_Properties;
		VkPhysicalDeviceFeatures VK_Phy_Device_Features;
		std::multimap<int, VkPhysicalDevice> rated_phy_devices_candidates;

		//Queues
		std::vector<VkQueueFamilyProperties> VK_Phy_Device_QueueFamilies;
		VkQueue VK_GraphicsQueue;
		VkQueue VK_PresentQueue;

		//logical devices
		VkDevice LogicalDevice;

		//surfaces
		VkSurfaceKHR VK_Surface;

		//Structs
		VkApplicationInfo VK_AppInfo{};
		VkInstanceCreateInfo VK_CreateInfo{};
		VkWin32SurfaceCreateInfoKHR VK_Surface_CreateInfo{};

		//Vulkan Extensions
		std::vector<VkExtensionProperties> VK_Available_Extensions;
		std::vector<const char*> VK_Extensions;

		//Device Extensions
		std::vector<VkExtensionProperties> VK_Available_Device_Extensions;
		std::vector<const char*> VK_Device_Extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		//SwapChain 
		SwapChainSupportDetails SwapChainSupport;
		VkSwapchainCreateInfoKHR VK_SwapChain_createInfo;
		VkSwapchainKHR VK_SwapChain;
		VkSurfaceFormatKHR format;
		VkExtent2D extent;
		VkPresentModeKHR presentMode;

		//SwapChain Images
		std::vector<VkImage> SwapChainImages;

		//SwapChain ImageViews
		std::vector<VkImageView> SwapChainImageViews;

		//shaders source codes
		std::map<std::string,std::pair<std::vector<char>,std::vector<char>>> shaders;

		//Shaders Modules
		std::vector<VkShaderModule> ShaderModules;

		//Shader Stages Creation Info
		VkPipelineShaderStageCreateInfo shaderStageCreateInfos[6];

		//Fixed Functions State Creation Info

		//Vertex input state
		VkPipelineVertexInputStateCreateInfo VertexInputInfo{};

		//Input assembly
		VkPipelineInputAssemblyStateCreateInfo InputAssembly{};

		//Viewport
		VkViewport viewport{};

		//Scissor
		VkRect2D scissor{};

		//Viewport & Scissor
		VkPipelineViewportStateCreateInfo ViewportState{};

		//Rasterizer
		VkPipelineRasterizationStateCreateInfo Rasterizer{};

		//Multisampling
		VkPipelineMultisampleStateCreateInfo multisampling{};

		//Depth & Stencil testing
		VkPipelineDepthStencilStateCreateInfo DepthStencil{};

		//Color Blending Attachment
		VkPipelineColorBlendAttachmentState ColorBlendAttachment{};

		//Color Blending State
		VkPipelineColorBlendStateCreateInfo ColorBlending{};

		//Dynamic State
		VkDynamicState DynamicStates[4] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH,
			VK_DYNAMIC_STATE_BLEND_CONSTANTS,
			VK_DYNAMIC_STATE_FRONT_FACE_EXT
		};
		VkPipelineDynamicStateCreateInfo DynamicState{};

		//Pipeline Layout
		VkPipelineLayout PipelineLayout;
		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo{};

		//Render Passes
		VkAttachmentDescription ColorAttachment{};


		//validation layers
#ifdef NDEBUG
		const bool enableValidationLayers = false;
#else
		const bool enableValidationLayers = true;
#endif
		uint32_t LayersCount = 0;
		std::vector<const char*> VK_ValidationLayers = {
			"VK_LAYER_KHRONOS_validation",
			"VK_LAYER_LUNARG_standard_validation" // LUNARG family and the standard as meta-layer are deprecated use ONLY KHRONOS standard
		};
		std::vector<VkLayerProperties> VK_AvailableValidationLayers;

		//Console
		HANDLE HConsole;

	public:

		void Render();

		std::string GetErrorName(size_t index);

		void PrintGLFWExtensions(std::vector<const char*> vec);

	};

};