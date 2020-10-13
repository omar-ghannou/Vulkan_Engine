#include "VRender.h"

VRender::VRender()
{
	pattern = DEVICE_PICKING_UP_PATTERN::USE_FIRST_SUITABLE_DEVICE;
	//ShowWindow(GetConsoleWindow(), SW_HIDE);
	VulkanLoadingStatus[VALIDATION_LAYERS] = ValidationState();
	VulkanLoadingStatus[VULKAN_LOADING] = Initiliazer();
	VulkanLoadingStatus[GLFW_TEST] = GLFWsetter();

	CreateInstance(); 
	SetupDebugMessenger();
	PickPhysicalDevice(VK_QUEUE_GRAPHICS_BIT);
	CreateLogicalDevice();
}

VRender::~VRender()
{
	vkDestroyDevice(LogicalDevice, nullptr); // device does not interact directly with the instance, that is why it is absent in the parameters
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
	if (!CheckForQueueFamily(device, bit).isComplete()) return false;

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
	if (!CheckForQueueFamily(device, bit).isComplete()) return 0;

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

QueueFamiliesIndices VRender::CheckForQueueFamily(VkPhysicalDevice device, VkQueueFlagBits bit)
{
	QueueFamiliesIndices indices{};
	std::vector<VkQueueFamilyProperties> device_queueFamily = FindQueueFamilies(device);
	int i = 0;
	for (const auto& queue_family : device_queueFamily) {
		if (queue_family.queueFlags & bit) indices.GraphicsFamily = i; 
		i++;
		if (indices.isComplete()) break;
	}
	return indices;
}

void VRender::CreateLogicalDevice()
{
	QueueFamiliesIndices indices = CheckForQueueFamily(PhysicalDevice, VK_QUEUE_GRAPHICS_BIT);
	VkDeviceQueueCreateInfo QueueCreateInfo{};
	QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	QueueCreateInfo.queueFamilyIndex = indices.GraphicsFamily.value();
	QueueCreateInfo.queueCount = 1;
	float QueuePriority = 1.0f;
	QueueCreateInfo.pQueuePriorities = &QueuePriority;

	VkPhysicalDeviceFeatures Device_features{};

	VkDeviceCreateInfo DeviceCreateInfo{};
	DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	DeviceCreateInfo.pQueueCreateInfos = &QueueCreateInfo;
	DeviceCreateInfo.queueCreateInfoCount = 1;
	DeviceCreateInfo.pEnabledFeatures = &Device_features;

	//later we will look ino the extensions
	//DeviceCreateInfo.enabledExtensionCount = VK_Extensions.size();
	//DeviceCreateInfo.ppEnabledExtensionNames = VK_Extensions.data();
	DeviceCreateInfo.enabledExtensionCount = 0;

	//previous implementation of vulkan separate between validation layers of a device and of an instance. 
	//now it is not the case, the device struct validation layers are ignored but for safety reasons implement them for older vulkan version support

	if (enableValidationLayers) {
		DeviceCreateInfo.enabledLayerCount = VK_ValidationLayers.size();
		DeviceCreateInfo.ppEnabledLayerNames = VK_ValidationLayers.data();
	}
	else DeviceCreateInfo.enabledLayerCount = 0;

	if (vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &LogicalDevice) != VK_SUCCESS)
		throw std::runtime_error("ERROR :: FAILD TO CREATE A LOGICAL DEVICE FROM THE PHYSICAL GPU");

	vkGetDeviceQueue(LogicalDevice, indices.GraphicsFamily.value(), 0, &VK_GraphicsQueue);
}

bool VRender::GLFWsetter()
{

	if (glfwInit() != GLFW_TRUE) {
		return false;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE,GLFW_FALSE);

	this->VK_Window = glfwCreateWindow(800,600,"Vulkan Render",nullptr,nullptr);

	if (VK_Window)
		glfwMakeContextCurrent(VK_Window);
	else return false;

	return true;
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
		VK_CreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&VK_Messenger_CreateInfo;
	}
	else 
	{
		VK_CreateInfo.enabledLayerCount = 0;
		VK_CreateInfo.pNext = nullptr;
	}

	PrintGLFWExtensions(VK_Extensions);

	if (static_cast<VkResult>(vkCreateInstance(&VK_CreateInfo, nullptr, &VK_Instance)) != VK_SUCCESS) {
			throw std::runtime_error("ERROR :: Faild to create a vulkan instance");
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
		//glfwPollEvents();
		glfwWaitEvents();
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
