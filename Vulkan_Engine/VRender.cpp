#include "VRender.h"

Vulkan_Engine::VRender::VRender()
{
	pattern = DEVICE_PICKING_UP_PATTERN::USE_FIRST_SUITABLE_DEVICE;
	HConsole = GetStdHandle(STD_OUTPUT_HANDLE);

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
	CreateImageView();
	CreateRenderPass();
	CreateGraphicsPipeline();
}

Vulkan_Engine::VRender::~VRender()
{
	vkDestroyPipelineLayout(LogicalDevice, PipelineLayout, nullptr);
	vkDestroyRenderPass(LogicalDevice, RenderPass, nullptr);
	for (auto& ImageView : SwapChainImageViews) {
		vkDestroyImageView(LogicalDevice, ImageView, nullptr);
	}
	vkDestroySwapchainKHR(LogicalDevice, VK_SwapChain, nullptr);
	vkDestroyDevice(LogicalDevice, nullptr); // device does not interact directly with the instance, that is why it is absent in the parameters
	vkDestroySurfaceKHR(VK_Instance, VK_Surface, nullptr);
	if (enableValidationLayers)
		DestroyDebugUtilsMessengerEXT(VK_Instance, debugMessenger, nullptr);
	vkDestroyInstance(VK_Instance, nullptr);
	glfwDestroyWindow(VK_Window);
	glfwTerminate();
}

VkResult Vulkan_Engine::VRender::CreateDebugUtilsMessengerEXT(const VkInstance& instance, VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void Vulkan_Engine::VRender::DestroyDebugUtilsMessengerEXT(const VkInstance& instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) func(instance, debugMessenger, pAllocator);
}

bool Vulkan_Engine::VRender::CheckValidationLayerSupport()
{
	vkEnumerateInstanceLayerProperties(&LayersCount, nullptr);
	VK_AvailableValidationLayers.resize(LayersCount);
	vkEnumerateInstanceLayerProperties(&LayersCount, VK_AvailableValidationLayers.data());

	SetConsoleTextAttribute(HConsole, 14);
	std::cout << "vulkan available validation layer\n\n";
	SetConsoleTextAttribute(HConsole, 15);

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

bool Vulkan_Engine::VRender::ValidationState()
{
	if (enableValidationLayers) {
		SetConsoleTextAttribute(HConsole, 6);
		std::cout << "Debug Mode : Enabled\n\n";
		SetConsoleTextAttribute(HConsole, 15);
		return CheckValidationLayerSupport();
	}
	return true;
}

void Vulkan_Engine::VRender::SetupDebugMessenger()
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
		SetConsoleTextAttribute(HConsole, 12);
		throw std::runtime_error("ERROR :: Failed to set up debug messenger");
		SetConsoleTextAttribute(HConsole, 15);
	}


}

bool Vulkan_Engine::VRender::CheckExtensionsBeforeInstance()
{
	uint32_t VK_ExtensionsCount;
	SetConsoleTextAttribute(HConsole, 14);
	std::cout << "Vulkan extensions \n\n";
	SetConsoleTextAttribute(HConsole, 15);
	vkEnumerateInstanceExtensionProperties(nullptr, &VK_ExtensionsCount, nullptr);
	VK_Available_Extensions.resize(VK_ExtensionsCount);
	if (vkEnumerateInstanceExtensionProperties(nullptr, &VK_ExtensionsCount, VK_Available_Extensions.data()) != VK_SUCCESS) return false;
	for (auto& VK_Extension : VK_Available_Extensions) {
		std::cout << VK_Extension.extensionName << '\t' << VK_Extension.specVersion << '\n';
	}
	return true;
}

std::vector<const char*> Vulkan_Engine::VRender::GLFWGetRequiredExtension()
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

bool Vulkan_Engine::VRender::CheckDeviceExtensionSupport(VkPhysicalDevice device)
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

void Vulkan_Engine::VRender::PickPhysicalDevice(VkQueueFlagBits bit)
{
	PhysicalDevice = VK_NULL_HANDLE;
	uint32_t devices_count = 0;
	vkEnumeratePhysicalDevices(VK_Instance, &devices_count, nullptr);
	if (!devices_count) {
		SetConsoleTextAttribute(HConsole, 12);
		throw std::runtime_error("ERROR :: CANNOT FIND ANY GPU THAT SUPPORTS VULKAN");
		SetConsoleTextAttribute(HConsole, 15);
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

			SetConsoleTextAttribute(HConsole, 14);
			std::cout << "\nphysical device extensions : \n";
			SetConsoleTextAttribute(HConsole, 15);

			for (const auto& extension : VK_Available_Device_Extensions) {
				std::cout << extension.extensionName << '\t' << extension.specVersion << '\n';
			}

		}
		else
		{
			SetConsoleTextAttribute(HConsole, 12);
			throw std::runtime_error("ERROR :: CANNOT FIND A SUITABLE GPU FOR THE APPLICATION IN THIS DEVICE");
			SetConsoleTextAttribute(HConsole, 15);
		}
	}


	if (pattern == DEVICE_PICKING_UP_PATTERN::USE_FIRST_SUITABLE_DEVICE)
	{
		for (auto& device : VK_Phy_Devices) {
			if (isDeviceSuitable(device,bit)) {
				PhysicalDevice = device;
				VK_Phy_Device_QueueFamilies = FindQueueFamilies(device);

				SetConsoleTextAttribute(HConsole, 14);
				std::cout << "\n\nPhysical device extensions : \n\n";
				SetConsoleTextAttribute(HConsole, 15);

				for (const auto& extension : VK_Available_Device_Extensions) {
					std::cout << extension.extensionName << '\t' << extension.specVersion << '\n';
				}
				break;
			}
		}

		if (PhysicalDevice == VK_NULL_HANDLE) {
			SetConsoleTextAttribute(HConsole, 12);
			throw std::runtime_error("ERROR :: CANNOT FIND A SUITABLE GPU FOR THE APPLICATION IN THIS DEVICE");
			SetConsoleTextAttribute(HConsole, 15);
		}
	}

}

bool Vulkan_Engine::VRender::isDeviceSuitable(VkPhysicalDevice device, VkQueueFlagBits bit)
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

int Vulkan_Engine::VRender::RateDeviceSuitability(VkPhysicalDevice device, VkQueueFlagBits bit)
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

std::vector<VkQueueFamilyProperties> Vulkan_Engine::VRender::FindQueueFamilies(VkPhysicalDevice device)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> device_queueFamily(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, device_queueFamily.data());
	return device_queueFamily;
}

Vulkan_Engine::QueueFamiliesIndices Vulkan_Engine::VRender::CheckForQueueFamily(VkPhysicalDevice device, VkQueueFlagBits bit, bool CheckForPresentQueue)
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

void Vulkan_Engine::VRender::CreateLogicalDevice()
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
	{
		SetConsoleTextAttribute(HConsole, 12);
		throw std::runtime_error("ERROR :: FAILD TO CREATE A LOGICAL DEVICE FROM THE PHYSICAL GPU");
		SetConsoleTextAttribute(HConsole, 15);
	}

	vkGetDeviceQueue(LogicalDevice, indices.GraphicsFamily.value(), 0, &VK_GraphicsQueue);
	vkGetDeviceQueue(LogicalDevice, indices.PresentFamily.value(), 0, &VK_PresentQueue);
	if (VK_GraphicsQueue == VK_PresentQueue)
	{
		SetConsoleTextAttribute(HConsole, 6);
		std::cout << "\nThe graphics and present queues are same\n\n";
		SetConsoleTextAttribute(HConsole, 15);
	}
}

void Vulkan_Engine::VRender::CreateSurface()
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
		SetConsoleTextAttribute(HConsole, 12);
		throw std::runtime_error("ERROR :: FAILED TO CREATE A SURFACE ON YOUR WINDOWING SYSTEM");
		SetConsoleTextAttribute(HConsole, 15);
	}

}

bool Vulkan_Engine::VRender::QuerySwapChainSupport(VkPhysicalDevice device)
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

VkSurfaceFormatKHR Vulkan_Engine::VRender::SelectSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& format : availableFormats) {
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) return format;
	}
	return availableFormats[0];
}

VkPresentModeKHR Vulkan_Engine::VRender::SelectSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availableModes)
{
	for (const auto& presentMode : availableModes) 
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) return presentMode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Vulkan_Engine::VRender::SelectSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities)
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

void Vulkan_Engine::VRender::CreateSwapChain()
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
		SetConsoleTextAttribute(HConsole, 12);
		throw std::runtime_error("ERROR :: FAILED TO CREATE A SWAP CHAIN FOR THE SURFACE");
		SetConsoleTextAttribute(HConsole, 15);
	}

	vkGetSwapchainImagesKHR(LogicalDevice, VK_SwapChain, &imageCount, nullptr);
	SwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(LogicalDevice, VK_SwapChain, &imageCount, SwapChainImages.data());

}

void Vulkan_Engine::VRender::CreateImageView()
{
	SwapChainImageViews.resize(SwapChainImages.size());

	for (size_t i = 0; i < SwapChainImages.size(); i++) 
	{
		VkImageViewCreateInfo ImageView_create_info{};
		ImageView_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageView_create_info.image = SwapChainImages[i];
		ImageView_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ImageView_create_info.format = format.format;

		ImageView_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageView_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageView_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageView_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		ImageView_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageView_create_info.subresourceRange.baseMipLevel = 0;
		ImageView_create_info.subresourceRange.levelCount = 1;
		ImageView_create_info.subresourceRange.baseArrayLayer = 0;
		ImageView_create_info.subresourceRange.layerCount = 1;

		if (vkCreateImageView(LogicalDevice, &ImageView_create_info, nullptr, &SwapChainImageViews[i]) != VK_SUCCESS) 
		{
			SetConsoleTextAttribute(HConsole, 12);
			throw std::runtime_error("ERROR :: FAILED TO CREATE IMAGE VIEWS FOR SWAP CHAIN IMAGES ");
			SetConsoleTextAttribute(HConsole, 15);
		}

	}
}

void Vulkan_Engine::VRender::LoadCompileShaders()
{
	char chi = 'n';
	std::cout << "Do you need to Load the shaders (y/n) : ";
	std::cin >> chi;
	if (chi == 'y' || chi == 'Y')
	{
		while (std::cin.get())
		{
			std::cout << "\nEnter the name of the shader(with extension), (quit) to quit: ";
			std::string name;
			std::cin >> name;
			if (name == "quit") break;
			std::cout << "Enter the type of the shader (vert/frag/tcs/tes/geom/comp) : ";
			std::string type;
			std::cin >> type;
			char ch = 'n';
			std::cout << "Do you need to compile/recompile the shader (y/n) : ";
			std::cin >> ch;
			if (ch == 'y' || ch == 'Y')
			{
				std::string command = "Shaders\\VulkanShaderCompiler.bat ";
				command.append(name);
				command.append(" ");
				command.append(type);
				system((const char*)command.c_str());
			}
			std::string path_to_glsl = "Shaders/";
			std::string path_to_spirv = "Shaders/SPIR-V/";
			std::vector<char> source[2];
			path_to_glsl.append(name);
			LoadShaderSource(path_to_glsl.c_str(), source[0]);
			std::string::size_type pos = name.find('.');
			std::string nameWEX = "";
			if (pos != std::string::npos)
			{
				nameWEX = name.substr(0, pos);
			}
			path_to_spirv.append(nameWEX);
			path_to_spirv.append(type);
			path_to_spirv.append(".spv");
			LoadShaderSource(path_to_spirv.c_str(), source[1]);

			std::pair<std::vector<char>, std::vector<char>> GLSL_SPIRV_map;
			std::pair pr = std::pair<std::vector<char>, std::vector<char>>(source[0], source[1]);
			shaders.insert(std::pair<std::string, std::pair<std::vector<char>, std::vector<char>>>(name, pr));

		}

	}
	else {
		SetConsoleTextAttribute(HConsole, 12);
		throw std::runtime_error("PROGRAM HAS BEEN STOPED :: THE SHADERS ARE NOT LOADED");
		SetConsoleTextAttribute(HConsole, 15);
	}
}

void Vulkan_Engine::VRender::PrintShadersMap()
{
	std::map<std::string, std::pair<std::vector<char>, std::vector<char>>>::reverse_iterator it = shaders.rbegin();

	SetConsoleTextAttribute(HConsole, 14);
	std::cout << "\n\nShaders map contains:\n\n";
	SetConsoleTextAttribute(HConsole, 11);

	for (it = shaders.rbegin(); it != shaders.rend(); ++it)
	{
		SetConsoleTextAttribute(HConsole, 9);
		std::cout << it->first << " \n\n ";
		SetConsoleTextAttribute(HConsole, 11);
		for (auto x : it->second.first) {
			std::cout << x;
		}

		std::cout << '\n' << std::endl;
	}
	SetConsoleTextAttribute(HConsole, 15);
}

VkShaderModule Vulkan_Engine::VRender::CreateShaderModule(const char* ShaderName,const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

	VkShaderModule ShaderModule;

	if (vkCreateShaderModule(LogicalDevice, &createInfo, nullptr, &ShaderModule) != VK_SUCCESS) {
		SetConsoleTextAttribute(HConsole, 12);
		std::string errorMessage = "ERROR :: FAILED TO CREATE SHADER MODULE FOR ";
		errorMessage.append(ShaderName);
		errorMessage.append(" Shader");
		throw std::runtime_error(errorMessage);
		SetConsoleTextAttribute(HConsole, 15);
	}
	return ShaderModule;
}

void Vulkan_Engine::VRender::CreateRenderPass()
{
	//Attachment Description
	ColorAttachment.format = format.format;
	ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	//Attachment Reference
	ColorAttachmentRef.attachment = 0;
	ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//Subpass
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &ColorAttachmentRef;

	//Render Pass
	RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	RenderPassCreateInfo.attachmentCount = 1;
	RenderPassCreateInfo.pAttachments = &ColorAttachment;
	RenderPassCreateInfo.subpassCount = 1;
	RenderPassCreateInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(LogicalDevice, &RenderPassCreateInfo, nullptr, &RenderPass) != VK_SUCCESS) {
		SetConsoleTextAttribute(HConsole, 12);
		throw std::runtime_error("ERROR :: FAILED TO CREATE RENDER PASSES");
		SetConsoleTextAttribute(HConsole, 15);
	}

}

void Vulkan_Engine::VRender::CreateGraphicsPipeline()
{
	LoadCompileShaders();
	PrintShadersMap();

	for (const auto& x : shaders) 
	{
		ShaderModules.push_back(CreateShaderModule(x.first.c_str(), x.second.second));
	}

	//shaders - programmable
	shaderStageCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStageCreateInfos[0].module = ShaderModules[1];
	shaderStageCreateInfos[0].pName = "main";

	shaderStageCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStageCreateInfos[1].module = ShaderModules[0];
	shaderStageCreateInfos[1].pName = "main";

	//Fixed functions

	//Vertex input state
	VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VertexInputInfo.vertexBindingDescriptionCount = 0;
	VertexInputInfo.pVertexBindingDescriptions = nullptr;
	VertexInputInfo.vertexAttributeDescriptionCount = 0;
	VertexInputInfo.pVertexAttributeDescriptions = nullptr;

	//Input assembly
	InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	InputAssembly.primitiveRestartEnable = VK_FALSE;

	//Viewport
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = extent.width;
	viewport.height = extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	//Scissor
	scissor.offset = { 0,0 };
	scissor.extent = extent;

	//Viewport State
	//in case of multi-viewport we need to enable this feature in the GPU (if supported) through the logical device, uncomment the next line to enable it.
	//VK_Phy_Device_Features.multiViewport = VK_TRUE; 
	ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	ViewportState.viewportCount = 1;
	ViewportState.pViewports = &viewport;
	ViewportState.scissorCount = 1;
	ViewportState.pScissors = &scissor;

	//Rasterizer
	Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	Rasterizer.depthClampEnable = VK_FALSE;
	Rasterizer.rasterizerDiscardEnable = VK_FALSE; // always discard the geometry through the rasterizer if it set to true
	Rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	Rasterizer.lineWidth = 1.0f;
	Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	Rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	Rasterizer.depthBiasEnable = VK_FALSE;
	Rasterizer.depthBiasConstantFactor = 0.0f;
	Rasterizer.depthBiasClamp = 0.0f;
	Rasterizer.depthBiasSlopeFactor = 0.0f;
	
	//Multisampling
	VK_Phy_Device_Features.samplerAnisotropy = VK_TRUE;
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	//Color Blending Attachment
	ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorBlendAttachment.blendEnable = VK_TRUE;
	ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
	ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	//Color Blending State
	ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	ColorBlending.logicOpEnable = VK_FALSE;
	ColorBlending.logicOp = VK_LOGIC_OP_COPY;
	ColorBlending.attachmentCount = 1;
	ColorBlending.pAttachments = &ColorBlendAttachment;
	ColorBlending.blendConstants[0] = 0.0f;
	ColorBlending.blendConstants[1] = 0.0f;
	ColorBlending.blendConstants[2] = 0.0f;
	ColorBlending.blendConstants[3] = 0.0f;

	//Dynamic States
	DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	DynamicState.dynamicStateCount = 4;
	DynamicState.pDynamicStates = DynamicStates;

	//Pipeline Layout
	PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PipelineLayoutCreateInfo.setLayoutCount = 0;
	PipelineLayoutCreateInfo.pSetLayouts = nullptr;
	PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	PipelineLayoutCreateInfo.pPushConstantRanges = nullptr;


	if (vkCreatePipelineLayout(LogicalDevice, &PipelineLayoutCreateInfo, nullptr, &PipelineLayout) != VK_SUCCESS) {
		SetConsoleTextAttribute(HConsole, 12);
		throw std::runtime_error("ERROR :: Failed to create the pipeline layout");
		SetConsoleTextAttribute(HConsole, 15);
	}

	for (auto& x : ShaderModules)
		vkDestroyShaderModule(LogicalDevice, x, nullptr);

	ShaderModules.~vector();


}

bool Vulkan_Engine::VRender::GLFWsetter()
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

bool Vulkan_Engine::VRender::Initiliazer()
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

void Vulkan_Engine::VRender::CreateInstance()
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
			SetConsoleTextAttribute(HConsole, 12);
			throw std::runtime_error("ERROR :: Failed to create a vulkan instance");
			SetConsoleTextAttribute(HConsole, 15);
	}


}

void Vulkan_Engine::VRender::Render()
{
	for (size_t test_index = 0; test_index < (sizeof(VulkanLoadingStatus) / sizeof(VulkanLoadingStatus[0])); test_index++)
	{
		if (VulkanLoadingStatus[test_index] == TEST_FAILD) {
			SetConsoleTextAttribute(HConsole, 12);
			std::cout << "\nERROR :: Failed to Load the render successfully. Please check " << GetErrorName(test_index) << std::endl;
			SetConsoleTextAttribute(HConsole, 15);
			return;
		}
	}

	while (!glfwWindowShouldClose(VK_Window)) {
		glfwPollEvents();
		//glfwWaitEvents();
	}

}

std::string Vulkan_Engine::VRender::GetErrorName(size_t index)
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

void Vulkan_Engine::VRender::PrintGLFWExtensions(std::vector<const char*> vec)
{
	SetConsoleTextAttribute(HConsole, 14);
	std::cout << "\n\nGLFW Vulkan required extensions \n\n";
	SetConsoleTextAttribute(HConsole, 15);

	for (auto& vec_element : vec) {
		std::cout << vec_element << '\n';
	}
}

bool Vulkan_Engine::VRender::LoadShaderSource(const char* path, std::string& src, int majorVersion, int minorVersion)
{
	std::ifstream shaderFile;
	shaderFile.open(path, std::ios::in | std::ios::binary);

	if (!shaderFile.is_open()) {
		std::string errorMessage = "ERROR :: COULD NOT LOAD THE FILE : ";
		errorMessage.append(path);
		SetConsoleTextAttribute(HConsole, 12);
		throw std::runtime_error(errorMessage);
		SetConsoleTextAttribute(HConsole, 15);
		return false;
	}
	//std::cout << "\n\nSource code of : " << path << '\n\n';

	std::string ver = "#version ";
	char v[3] = {'1','2','0'};
	_itoa_s(majorVersion * 100 + minorVersion * 10, v, 10);
	ver.append(v,3);

	while (!shaderFile.eof()) {
		char line[256];
		shaderFile.getline(line,256);
		if (line[0] == '#' && line[1] == 'v' && line[2] == 'e')
		{
			src.append(ver);
		}
		else
			src.append(line);
		//std::cout << line << '\n';
	}

	shaderFile.close();

	return true;
}

bool Vulkan_Engine::VRender::LoadShaderSource(const char* path, std::vector<char>& src)
{
	std::ifstream shaderFile;
	shaderFile.open(path, std::ios::in | std::ios::binary | std::ios::ate);

	if (!shaderFile.is_open()) {
		std::string errorMessage = "ERROR :: COULD NOT LOAD THE FILE : ";
		errorMessage.append(path);
		SetConsoleTextAttribute(HConsole, 12);
		throw std::runtime_error(errorMessage);
		SetConsoleTextAttribute(HConsole, 15);
		return false;
	}

	size_t fileSize = (size_t)shaderFile.tellg();
	src.resize(fileSize);

	shaderFile.seekg(shaderFile._Seekbeg);
	shaderFile.read(src.data(), fileSize);

	shaderFile.close();

	return true;

}

bool Vulkan_Engine::VRender::LoadShaderSource(const char* path, std::vector<char>& src, int majorVersion, int minorVersion)
{
	std::ifstream shaderFile;
	shaderFile.open(path, std::ios::in | std::ios::binary | std::ios::ate);

	if (!shaderFile.is_open()) {
		std::string errorMessage = "ERROR :: COULD NOT LOAD THE FILE : ";
		errorMessage.append(path);
		SetConsoleTextAttribute(HConsole, 12);
		throw std::runtime_error(errorMessage);
		SetConsoleTextAttribute(HConsole, 15);
		return false;
	}

	//std::cout << "\n\nSource code of : " << path << '\n\n';

	std::string ver = "#version ";
	char v[3] = { '1','2','0' };
	_itoa_s(majorVersion * 100 + minorVersion * 10, v, 10);
	ver.append(v, 3);

	size_t fileSize = (size_t)shaderFile.tellg();
	src.resize(fileSize);

	shaderFile.seekg(shaderFile._Seekbeg);
	shaderFile.read(src.data(), fileSize);

	shaderFile.close();

	return true;

}
