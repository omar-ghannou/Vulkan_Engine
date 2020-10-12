#include "VRender.h"

VRender::VRender()
{
	//ShowWindow(GetConsoleWindow(), SW_HIDE);
	VulkanLoadingStatus[VALIDATION_LAYERS] = ValidationState();
	VulkanLoadingStatus[VULKAN_LOADING] = Initiliazer();
	VulkanLoadingStatus[GLFW_TEST] = GLFWsetter();
	CreateInstance(); 
	SetupDebugMessenger();
	PickPhysicalDevice();


}

VRender::~VRender()
{
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

	try {
		if (CreateDebugUtilsMessengerEXT(VK_Instance, &VK_Messenger_CreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("Failed to set up debug messenger");
		}
	}
	catch (std::exception& e) {
		std::cout << e.what();
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

	try {
		if (static_cast<VkResult>(vkCreateInstance(&VK_CreateInfo, nullptr, &VK_Instance)) != VK_SUCCESS) {
			throw std::runtime_error("Faild to create a vulkan instance");
		}
	}
	catch (std::exception& e) {
		std::cout << '\n' <<e.what() << '\n';
	}


}

void VRender::Render()
{
	for (size_t test_index = 0; test_index < (sizeof(VulkanLoadingStatus) / sizeof(VulkanLoadingStatus[0])); test_index++)
	{
		if (VulkanLoadingStatus[test_index] == TEST_FAILD) {
			std::cout << "\nFailed to Load the render successfully. Please check " << GetErrorName(test_index) << std::endl;
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
