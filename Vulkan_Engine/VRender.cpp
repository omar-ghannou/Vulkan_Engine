#include "VRender.h"

VRender::VRender()
{
	pattern = DEVICE_PICKING_UP_PATTERN::USE_FIRST_SUITABLE_DEVICE;

	//ShowWindow(GetConsoleWindow(), SW_HIDE);
	VulkanLoadingStatus[VULKAN_LOADING] = Initiliazer();
	VulkanLoadingStatus[VALIDATION_LAYERS] = ValidationState();
	VulkanLoadingStatus[GLFW_TEST] = GLFWsetter();

	CreateInstance(); 
	SetupDebugMessenger();
	CreateSurface(); //surface creation should take a place before physical device picking up, because it may affect the results if it is after
	PickPhysicalDevice(VK_QUEUE_GRAPHICS_BIT);
	CreateLogicalDevice();
	CreateSwapChain();
}

VRender::~VRender()
{
	vkDestroySwapchainKHR(LogicalDevice, VK_SwapChain, nullptr);
	vkDestroyDevice(LogicalDevice, nullptr); // device does not interact directly with the instance, that is why it is absent in the parameters
	vkDestroySurfaceKHR(VK_Instance, VK_Surface, nullptr);
	if (enableValidationLayers)
		DestroyDebugUtilsMessengerEXT(VK_Instance, debugMessenger, nullptr);
	vkDestroyInstance(VK_Instance, nullptr);
	glfwDestroyWindow(VK_Window);
	glfwTerminate();
}

VkResult VRender::CreateDebugUtilsMessengerEXT(const VkInstance& instance, VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VRender::DestroyDebugUtilsMessengerEXT(const VkInstance& instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) func(instance, debugMessenger, pAllocator);
}

bool VRender::CheckValidationLayerSupport()
{
	vkEnumerateInstanceLayerProperties(&LayersCount, nullptr);
	VK_AvailableValidationLayers.resize(LayersCount);
	vkEnumerateInstanceLayerProperties(&LayersCount, VK_AvailableValidationLayers.data());

	std::cout << "vulkan available validation layer\n\n";
	for (auto& LayerProperties : VK_AvailableValidationLayers) {
		std::cout << LayerProperties.layerName << '\t' << LayerProperties.description << '\t' << LayerProperties.implementationVersion << '\n';
	}
	std::cout << std::endl;
	for (auto& LayerName : VK_ValidationLayers) {
		bool LayerFound = false;
		for (auto& LayerProperties : VK_AvailableValidationLayers) {
			if (strcmp(LayerProperties.layerName,LayerName) == 0)
			{
				LayerFound = true;
				break;
			}
		}
		if (!LayerFound) return false;
	}

	return true;
}

bool VRender::ValidationState()
{
	if (enableValidationLayers) {
		std::cout << "Debug Mode enabled\n\n";
		return CheckValidationLayerSupport();
	}
	return true;
}

void VRender::SetupDebugMessenger()
{
	if (!enableValidationLayers) return;

	VK_Messenger_CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	VK_Messenger_CreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	VK_Messenger_CreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	VK_Messenger_CreateInfo.pfnUserCallback = DebugCallback;
	VK_Messenger_CreateInfo.pUserData = nullptr;


	if (CreateDebugUtilsMessengerEXT(VK_Instance, &VK_Messenger_CreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("ERROR :: Failed to set up debug messenger");
	}


}

bool VRender::CheckExtensionsBeforeInstance()
{
	uint32_t VK_ExtensionsCount;
	std::cout << "Vulkan extensions \n\n";
	vkEnumerateInstanceExtensionProperties(nullptr, &VK_ExtensionsCount, nullptr);
	VK_Available_Extensions.resize(VK_ExtensionsCount);
	if (vkEnumerateInstanceExtensionProperties(nullptr, &VK_ExtensionsCount, VK_Available_Extensions.data()) != VK_SUCCESS) return false;
	for (auto& VK_Extension : VK_Available_Extensions) {
		std::cout << VK_Extension.extensionName << '\t' << VK_Extension.specVersion << '\n';
	}
	return true;
}

std::vector<const char*> VRender::GLFWGetRequiredExtension()
{
	uint32_t GLFW_VK_ExtensionCount;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&GLFW_VK_ExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + GLFW_VK_ExtensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		GLFW_VK_ExtensionCount++;
	}

	return extensions;
}

bool VRender::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t DeviceExtensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &DeviceExtensionCount, nullptr);
	VK_Available_Device_Extensions.resize(DeviceExtensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &DeviceExtensionCount, VK_Available_Device_Extensions.data());

	std::set<std::string> RequiredExtension(VK_Device_Extensions.begin(), VK_Device_Extensions.end());

	for (const auto& extension : VK_Available_Device_Extensions) {
		RequiredExtension.erase(extension.extensionName);
	}

	return RequiredExtension.empty();
}

void VRender::PickPhysicalDevice(VkQueueFlagBits bit)
{
	PhysicalDevice = VK_NULL_HANDLE;
	uint32_t devices_count = 0;
	vkEnumeratePhysicalDevices(VK_Instance, &devices_count, nullptr);
	if (!devices_count) {
		throw std::runtime_error("ERROR :: CANNOT FIND ANY GPU THAT SUPPORTS VULKAN");
	}
	VK_Phy_Devices.resize(devices_count);
	vkEnumeratePhysicalDevices(VK_Instance, &devices_count, VK_Phy_Devices.data());



	if (pattern == DEVICE_PICKING_UP_PATTERN::USE_BEST_RATED_SUITABLE_DEVICE)
	{
		
		for (const auto& device : VK_Phy_Devices) {
			int score = RateDeviceSuitability(device,bit);
			rated_phy_devices_candidates.insert(std::make_pair(score, device));
		}

		if (rated_phy_devices_candidates.rbegin()->first > 0)
		{
			PhysicalDevice = rated_phy_devices_candidates.rbegin()->second;
			VK_Phy_Device_QueueFamilies = FindQueueFamilies(PhysicalDevice);
			vkGetPhysicalDeviceProperties(PhysicalDevice, &VK_Phy_Device_Properties);
			vkGetPhysicalDeviceFeatures(PhysicalDevice, &VK_Phy_Device_Features);

			std::cout << "\nphysical device extensions : \n";
			for (const auto& extension : VK_Available_Device_Extensions) {
				std::cout << extension.extensionName << '\t' << extension.specVersion << '\n';
			}

		}
		else
		{
			throw std::runtime_error("ERROR :: CANNOT FIND A SUITABLE GPU FOR THE APPLICATION IN THIS DEVICE");
		}
	}


	if (pattern == DEVICE_PICKING_UP_PATTERN::USE_FIRST_SUITABLE_DEVICE)
	{
		for (auto& device : VK_Phy_Devices) {
			if (isDeviceSuitable(device,bit)) {
				PhysicalDevice = device;
				VK_Phy_Device_QueueFamilies = FindQueueFamilies(device);
				std::cout << "\n\nPhysical device extensions : \n\n";
				for (const auto& extension : VK_Available_Device_Extensions) {
					std::cout << extension.extensionName << '\t' << extension.specVersion << '\n';
				}
				break;
			}
		}

		if (PhysicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("ERROR :: CANNOT FIND A SUITABLE GPU FOR THE APPLICATION IN THIS DEVICE");
		}
	}

}

bool VRender::isDeviceSuitable(VkPhysicalDevice device, VkQueueFlagBits bit)
{
	//this function should be more dynamique and programmable
	if (!CheckForQueueFamily(device, bit, true).isComplete()) return false;

	if (!CheckDeviceExtensionSupport(device)) return false;

	if (!QuerySwapChainSupport(device)) return false;

	VkPhysicalDeviceProperties device_properties;
	VkPhysicalDeviceFeatures device_features;

	vkGetPhysicalDeviceProperties(device, &device_properties);
	vkGetPhysicalDeviceFeatures(device, &device_features);

	

	if ((device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU || device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ) && device_features.geometryShader) {
		VK_Phy_Device_Properties = device_properties;
		VK_Phy_Device_Features = device_features;
		return true;
	}
	else
	return false;
}

int VRender::RateDeviceSuitability(VkPhysicalDevice device, VkQueueFlagBits bit)
{
	//this function should be more dynamique and programmable

	if (!CheckForQueueFamily(device, bit, true).isComplete()) return 0;

	if (!CheckDeviceExtensionSupport(device)) return 0;

	if (!QuerySwapChainSupport(device)) return 0;

	VkPhysicalDeviceProperties device_properties;
	VkPhysicalDeviceFeatures device_features;

	vkGetPhysicalDeviceProperties(device, &device_properties);
	vkGetPhysicalDeviceFeatures(device, &device_features);
	int score = 0;
	
	// Discrete GPUs have a significant performance advantage
	if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;
	else if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) score += 250;
	 // Maximum possible size of textures affects graphics quality
	score += device_properties.limits.maxImageDimension2D;
	// Application can't function without geometry shaders
	if (!device_features.geometryShader) 
	return 0;	
	

	return score;

}

std::vector<VkQueueFamilyProperties> VRender::FindQueueFamilies(VkPhysicalDevice device)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> device_queueFamily(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, device_queueFamily.data());
	return device_queueFamily;
}

QueueFamiliesIndices VRender::CheckForQueueFamily(VkPhysicalDevice device, VkQueueFlagBits bit, bool CheckForPresentQueue)
{
	QueueFamiliesIndices indices{};
	std::vector<VkQueueFamilyProperties> device_queueFamily = FindQueueFamilies(device);
	int i = 0;
	for (const auto& queue_family : device_queueFamily) {

		if (queue_family.queueFlags & bit) {
			indices.GraphicsFamily = i;
			i++;
			break;
		}
	}
	if (CheckForPresentQueue)
	{
		if (i >= device_queueFamily.size()) i = indices.GraphicsFamily.value(); // PresentQueueIndex must be less than queueFamilyCountProperty returned by the function in FindQueueFamilies
		VkBool32 PresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, VK_Surface, &PresentSupport); //this may end up to be same queue as Graphics queue and we can explicity prefer them to be same for improved performance
		if (PresentSupport) indices.PresentFamily = i;
	}
	return indices;
}

void VRender::CreateLogicalDevice()
{
	QueueFamiliesIndices indices = CheckForQueueFamily(PhysicalDevice, VK_QUEUE_GRAPHICS_BIT, true);

	std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
	std::set<uint32_t> UniqueQueueFamilies = { indices.GraphicsFamily.value(),indices.PresentFamily.value() };
	float QueuePriority = 1.0f;
	for (const auto& queue : UniqueQueueFamilies) {
		VkDeviceQueueCreateInfo QueueCreateInfo{};
		QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueCreateInfo.queueFamilyIndex = queue;
		QueueCreateInfo.queueCount = 1;
		QueueCreateInfo.pQueuePriorities = &QueuePriority;
		QueueCreateInfos.push_back(QueueCreateInfo);
	}

	VkPhysicalDeviceFeatures Device_features{};//we can use the ones selected and stored in VK_Phy_Device_Features variable, but no need for now

	VkDeviceCreateInfo DeviceCreateInfo{};
	DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	DeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreateInfos.size());
	DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
	DeviceCreateInfo.pEnabledFeatures = &Device_features;

	if (!VK_Device_Extensions.empty()) 
	{
		DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(VK_Device_Extensions.size());
		DeviceCreateInfo.ppEnabledExtensionNames = VK_Device_Extensions.data();
	}
	else DeviceCreateInfo.enabledExtensionCount = 0;

	//previous implementation of vulkan separate between validation layers of a device and of an instance. 
	//now it is not the case, the device struct validation layers are ignored but for safety reasons implement them for older vulkan version support

	if (enableValidationLayers) {
		DeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VK_ValidationLayers.size());
		DeviceCreateInfo.ppEnabledLayerNames = VK_ValidationLayers.data();
	}
	else DeviceCreateInfo.enabledLayerCount = 0;

	if (vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &LogicalDevice) != VK_SUCCESS)
		throw std::runtime_error("ERROR :: FAILD TO CREATE A LOGICAL DEVICE FROM THE PHYSICAL GPU");

	vkGetDeviceQueue(LogicalDevice, indices.GraphicsFamily.value(), 0, &VK_GraphicsQueue);
	vkGetDeviceQueue(LogicalDevice, indices.PresentFamily.value(), 0, &VK_PresentQueue);
	if (VK_GraphicsQueue == VK_PresentQueue) std::cout << "\nThe graphics and present queues are same\n";
}

void VRender::CreateSurface()
{
	//rather you can avoid this native implemetation and you glfwCreateWindowSurface function to create a surface
	//the same way I did but it has a diffrent implementaion for each platform
	//for linux replace WIN32 in all surface related commands to XCB

	VK_Surface_CreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	VK_Surface_CreateInfo.hwnd = glfwGetWin32Window(VK_Window);
	VK_Surface_CreateInfo.hinstance = GetModuleHandle(nullptr);

	//vkCreateWin32SurfaceKHR is an-extension-based function, but it is so commonly that is why it is in the standard
	//it does not need to be loaded explicitly
	if (vkCreateWin32SurfaceKHR(VK_Instance, &VK_Surface_CreateInfo, nullptr, &VK_Surface) != VK_SUCCESS) 
	{
		throw std::runtime_error("ERROR :: FAILED TO CREATE A SURFACE ON YOUR WINDOWING SYSTEM");
	}

}

bool VRender::QuerySwapChainSupport(VkPhysicalDevice device)
{
	
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, VK_Surface, &SwapChainSupport.SurfaceCapabilities);

	uint32_t FormatsCount = 0;

	vkGetPhysicalDeviceSurfaceFormatsKHR(device, VK_Surface, &FormatsCount, nullptr);
	if (FormatsCount)
	{
		SwapChainSupport.SurfaceFormats.resize(FormatsCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, VK_Surface, &FormatsCount, SwapChainSupport.SurfaceFormats.data());
	}

	uint32_t PresentModeCount = 0;

	vkGetPhysicalDeviceSurfacePresentModesKHR(device, VK_Surface, &PresentModeCount, nullptr);
	if (PresentModeCount)
	{
		SwapChainSupport.SurfacePresentMode.resize(PresentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, VK_Surface, &PresentModeCount, SwapChainSupport.SurfacePresentMode.data());
	}

	return (!SwapChainSupport.SurfaceFormats.empty() && !SwapChainSupport.SurfacePresentMode.empty());

}

VkSurfaceFormatKHR VRender::SelectSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& format : availableFormats) {
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) return format;
	}
	return availableFormats[0];
}

VkPresentModeKHR VRender::SelectSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availableModes)
{
	for (const auto& presentMode : availableModes) 
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) return presentMode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VRender::SelectSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if(capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D actualExtent = { 1080,720 };
		
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
		return actualExtent;
	}
}

void VRender::CreateSwapChain()
{
	format = SelectSwapChainFormat(SwapChainSupport.SurfaceFormats);
	extent = SelectSwapChainExtent(SwapChainSupport.SurfaceCapabilities);
	presentMode = SelectSwapChainPresentMode(SwapChainSupport.SurfacePresentMode);

	uint32_t imageCount = SwapChainSupport.SurfaceCapabilities.minImageCount + 1; //take min+1 to avoid waiting on the driver 
	if (SwapChainSupport.SurfaceCapabilities.maxImageCount > 0 && imageCount > SwapChainSupport.SurfaceCapabilities.maxImageCount)
		imageCount = SwapChainSupport.SurfaceCapabilities.maxImageCount;

	VK_SwapChain_createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	VK_SwapChain_createInfo.surface = VK_Surface;
	VK_SwapChain_createInfo.minImageCount = imageCount;
	VK_SwapChain_createInfo.imageFormat = format.format;
	VK_SwapChain_createInfo.imageColorSpace = format.colorSpace;
	VK_SwapChain_createInfo.imageExtent = extent;
	VK_SwapChain_createInfo.imageArrayLayers = 1; //always 1 unless developing stereoscopic 3D
	VK_SwapChain_createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamiliesIndices indices = CheckForQueueFamily(PhysicalDevice, VK_QUEUE_GRAPHICS_BIT, true);
	uint32_t QueueIndices[] = { indices.GraphicsFamily.value(),indices.PresentFamily.value() };

	if (indices.GraphicsFamily != indices.PresentFamily) 
	{
		VK_SwapChain_createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; //Images can be used across multiple queue families without explicit ownership transfers.
		VK_SwapChain_createInfo.queueFamilyIndexCount = 2;
		VK_SwapChain_createInfo.pQueueFamilyIndices = QueueIndices;
	}
	else
	{
		VK_SwapChain_createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //An image is owned by one queue family at a time and ownership must be explicitly transfered before using it in another queue family. This option offers the best performance.
		VK_SwapChain_createInfo.queueFamilyIndexCount = 0;
		VK_SwapChain_createInfo.pQueueFamilyIndices = nullptr;
	}

	VK_SwapChain_createInfo.preTransform = SwapChainSupport.SurfaceCapabilities.currentTransform; //no transforms specified
	VK_SwapChain_createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	VK_SwapChain_createInfo.presentMode = presentMode;

	VK_SwapChain_createInfo.clipped = VK_TRUE;

	VK_SwapChain_createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(LogicalDevice, &VK_SwapChain_createInfo, nullptr, &VK_SwapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR :: FAILED TO CREATE A SWAP CHAIN FOR THE SURFACE");
	}

	vkGetSwapchainImagesKHR(LogicalDevice, VK_SwapChain, &imageCount, nullptr);
	SwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(LogicalDevice, VK_SwapChain, &imageCount, SwapChainImages.data());

}

bool VRender::GLFWsetter()
{

	if (glfwInit() != GLFW_TRUE) {
		return false;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE,GLFW_FALSE);

	this->VK_Window = glfwCreateWindow(800,600,"Vulkan Render",nullptr,nullptr);

	if (VK_Window) return true;
	else return false;
}

bool VRender::Initiliazer()
{
#if defined _WIN32 
	//HINSTANCE vulkan_library = LoadLibraryA("vulkan-1.dll");
#elif defined __linux 
	//vulkan_library = dlopen("libvulkan.so.1", RTLD_NOW);
#endif 

	
	//if (vulkan_library == nullptr) {
	//	std::cout << "Could not connect with a Vulkan Runtime library." << std::endl;
	//	return false;
	//}
	return true;
}

void VRender::CreateInstance()
{
	VK_Messenger_CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT; // to avoid debugger warning about pNext in the instance creation, might I'll make a function to initialize all the structures

	CheckExtensionsBeforeInstance();

	//AppInfo
	VK_AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	VK_AppInfo.pApplicationName = "Vulkan Render";
	VK_AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	VK_AppInfo.pEngineName = "Vulkan Engine";
	VK_AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	VK_AppInfo.apiVersion = VK_API_VERSION_1_0;


	//Instance Info
	VK_CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	VK_CreateInfo.pApplicationInfo = &VK_AppInfo;

	VK_Extensions = GLFWGetRequiredExtension();

	if (!VK_Extensions.empty()) {
		VK_CreateInfo.enabledExtensionCount = static_cast<uint32_t>(VK_Extensions.size());
		VK_CreateInfo.ppEnabledExtensionNames = VK_Extensions.data();
	}

	if (enableValidationLayers)
	{
		VK_CreateInfo.enabledLayerCount = static_cast<uint32_t>(VK_ValidationLayers.size());
		VK_CreateInfo.ppEnabledLayerNames = VK_ValidationLayers.data();
		VK_CreateInfo.pNext = dynamic_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&VK_Messenger_CreateInfo);
	}
	else 
	{
		VK_CreateInfo.enabledLayerCount = 0;
		VK_CreateInfo.pNext = nullptr;
	}

	PrintGLFWExtensions(VK_Extensions);

	if (static_cast<VkResult>(vkCreateInstance(&VK_CreateInfo, nullptr, &VK_Instance)) != VK_SUCCESS) {
			throw std::runtime_error("ERROR :: Failed to create a vulkan instance");
	}


}

void VRender::Render()
{
	for (size_t test_index = 0; test_index < (sizeof(VulkanLoadingStatus) / sizeof(VulkanLoadingStatus[0])); test_index++)
	{
		if (VulkanLoadingStatus[test_index] == TEST_FAILD) {
			std::cout << "\nERROR :: Failed to Load the render successfully. Please check " << GetErrorName(test_index) << std::endl;
			return;
		}
	}

	while (!glfwWindowShouldClose(VK_Window)) {
		glfwPollEvents();
		//glfwWaitEvents();
	}

}

std::string VRender::GetErrorName(size_t index)
{
	switch (index)
	{
	case 0:
		return "GLFW_TEST";
		break;
	case 1:
		return "VULKAN_LOADER";
		break;
	case 2: 
		return "VALIDATION_LAYERS";
		break;
	default:
		return "NONE";
		break;
	}
	return "";
}

void VRender::PrintGLFWExtensions(std::vector<const char*> vec)
{
	std::cout << "\n\nGLFW Vulkan required extensions \n\n";
	for (auto& vec_element : vec) {
		std::cout << vec_element << '\n';
	}
}
