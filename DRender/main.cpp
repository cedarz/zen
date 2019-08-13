#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <exception>
#include <optional>
#include <cstring>
#include <set>
#include <algorithm>
#include <fstream>
#include <vulkan/vulkan.h>

const std::vector<const char*> kValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
};
const std::vector<const char*> kDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
const size_t kMaxFramesInFlight = 2;

#ifdef _DEBUG
const bool kEnableValidationLayers = true;
#else
const bool kEnableValidationLayers = false;
#endif // DEBUG

struct QueueFamilyIndex {
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;
	bool IsComplete() {
		return graphics_family.has_value() && present_family.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

class VkDRender {
public:
	explicit VkDRender(int w, int h) : width(h), height(h) {}

	void Run() {
		InitWindow();
		InitVulkan();
		MainLoop();
		CleanUp();
	}

private:

	void InitWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		//window = glfwCreateWindow(width, height, "Vulkan Deferred Renderer", nullptr, nullptr);
		window = glfwCreateWindow(800, 600, "Vulkan Deferred Renderer", nullptr, nullptr);
	}

	void InitVulkan() {
		CreateInstance();
		CreateDebugMessengerExt();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateRenderPass();
		CreateGraphicsPileline();
		CreateFramebuffers();
		CreateCommandPool();
		CreateCommandBuffers();
		CreateSyncObjects();
	}

	void MainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			DrawFrame();
		}
		vkDeviceWaitIdle(vk_logical_device);
	}

	void CleanUp() {
		for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
			vkDestroySemaphore(vk_logical_device, vk_image_available_semaphores[i], nullptr);
			vkDestroySemaphore(vk_logical_device, vk_reder_finished_semaphores[i], nullptr);
			vkDestroyFence(vk_logical_device, vk_fences[i], nullptr);
		}
		vkDestroyCommandPool(vk_logical_device, vk_command_pool, nullptr);
		for (auto fb : vk_framebuffers) {
			vkDestroyFramebuffer(vk_logical_device, fb, nullptr);
		}
		vkDestroyPipeline(vk_logical_device, vk_pipeline, nullptr);
		vkDestroyPipelineLayout(vk_logical_device, vk_pipeline_layout,nullptr);
		vkDestroyRenderPass(vk_logical_device, vk_render_pass, nullptr);

		for (auto img_view : vk_swapchain_image_views) {
			vkDestroyImageView(vk_logical_device, img_view, nullptr);
		}
		vkDestroySwapchainKHR(vk_logical_device, vk_swapchain, nullptr);
		vkDestroyDevice(vk_logical_device, nullptr);

		if (kEnableValidationLayers) {
			DestroyDebugMessengerExt();
		}
		vkDestroySurfaceKHR(vk_instance, vk_surface, nullptr);
		vkDestroyInstance(vk_instance, nullptr);

		glfwDestroyWindow(window);
		glfwTerminate();
	}
private:
	// static functions
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {
		std::cerr << "Validation Layer: " << callback_data->pMessage << std::endl;
		return VK_FALSE;
	}
	
	static std::vector<char> ReadFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file!");
		}
		size_t file_size = (size_t)file.tellg();
		std::vector<char> buffer(file_size);
		file.seekg(0);
		file.read(buffer.data(), file_size);
		file.close();
		return buffer;
	}
	bool CheckValidationLayerSupport() {
		uint32_t layer_count = 0;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
		std::vector<VkLayerProperties> layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, layers.data());

		for (const char* layer_name : kValidationLayers) {
			bool layer_found = false;
			for (const auto& layer_prop : layers) {
				if (strcmp(layer_name, layer_prop.layerName) == 0) {
					layer_found = true;
					break;
				}
			}
			if (!layer_found) {
				return false;
			}
		}
		return true;
	}

	std::vector<const char*> GetExtensions() {
		uint32_t glfw_extension_count = 0;
		const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
		std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
		if (kEnableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}

	void InitializeDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT & debug_create_info) {
		debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debug_create_info.pfnUserCallback = DebugUtilsCallback;
	}

	void SelectPhysicalDeviceQueueFamilyIndex(VkPhysicalDevice physical_device) {
		//QueueFamilyIndex index;
		vk_queue_family_index = QueueFamilyIndex{};
		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
		std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

		for (int idx = 0; idx < queue_families.size(); ++idx) {
			auto& queue_fam = queue_families[idx];
			if (queue_fam.queueCount > 0 && queue_fam.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				vk_queue_family_index.graphics_family = idx;
			}

			VkBool32 present_support = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, idx, vk_surface, &present_support);
			if (queue_fam.queueCount > 0 && present_support) {
				vk_queue_family_index.present_family = idx;
			}

			if (vk_queue_family_index.IsComplete()) {
				break;
			}
		}
	}

	void SelectPhysicalDeviceSwapChainSupportDetails(VkPhysicalDevice physical_device) {
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, vk_surface, &vk_swapchain_support_details.capabilities);
		uint32_t format_count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, vk_surface, &format_count, nullptr);
		if (format_count > 0) {
			vk_swapchain_support_details.formats.resize(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, vk_surface, &format_count, vk_swapchain_support_details.formats.data());
		}

		uint32_t present_mode_count = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, vk_surface, &present_mode_count, nullptr);
		if (present_mode_count > 0) {
			vk_swapchain_support_details.present_modes.resize(present_mode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, vk_surface, &present_mode_count, vk_swapchain_support_details.present_modes.data());;
		}
	}

	bool CheckPhysicalDeviceExtensionsSupport(VkPhysicalDevice physical_device) {
		uint32_t extension_count = 0;
		vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);
		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions.data());

		std::set<std::string> required_extensions(kDeviceExtensions.begin(), kDeviceExtensions.end());
		for (const auto& ext_prop : available_extensions) {
			required_extensions.erase(ext_prop.extensionName);
		}
		return required_extensions.empty();
	}

	bool CheckPhysicalDeviceAdequate(VkPhysicalDevice physical_device) {
		SelectPhysicalDeviceQueueFamilyIndex(physical_device);
		bool extensions_supported = CheckPhysicalDeviceExtensionsSupport(physical_device);
		bool swapchain_adequate = false;
		if (extensions_supported) {
			SelectPhysicalDeviceSwapChainSupportDetails(physical_device);
			swapchain_adequate = !vk_swapchain_support_details.formats.empty() && !vk_swapchain_support_details.present_modes.empty();
		}
		return vk_queue_family_index.IsComplete() && extensions_supported && swapchain_adequate;
	}
private:
	// function helpers
	void CreateInstance() {
		if (kEnableValidationLayers && !CheckValidationLayerSupport()) {
			throw std::runtime_error("Validation Layer requested, but not available!");
		}
		VkApplicationInfo app_info = {};
		{
			app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			app_info.pApplicationName = "Vulkan Deferred Renderer";
			app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
			app_info.pEngineName = "VK_DR";
			app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
			app_info.apiVersion = VK_API_VERSION_1_1;
		}

		VkInstanceCreateInfo instance_create_info = {};
		std::vector<const char*> extensions = GetExtensions();
		{
			instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instance_create_info.pApplicationInfo = &app_info;

			instance_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
			instance_create_info.ppEnabledExtensionNames = extensions.data();

			if (kEnableValidationLayers) {
				instance_create_info.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
				instance_create_info.ppEnabledLayerNames = kValidationLayers.data();

				VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
				InitializeDebugMessengerCreateInfo(debug_create_info);
				instance_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
			} else {
				instance_create_info.enabledLayerCount = 0;
				instance_create_info.pNext = nullptr;
			}
			
		}

		if (vkCreateInstance(&instance_create_info, nullptr, &vk_instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	void CreateDebugMessengerExt() {
		if (kEnableValidationLayers) {
			VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
			InitializeDebugMessengerCreateInfo(debug_create_info);

			auto CreateDebugUtilsMessengerExtFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkCreateDebugUtilsMessengerEXT");
			//vkCreateDebugUtilsMessengerEXT
			if (!CreateDebugUtilsMessengerExtFunc || CreateDebugUtilsMessengerExtFunc(vk_instance, &debug_create_info, nullptr, &vk_debug_messenger) != VK_SUCCESS) {
				throw std::runtime_error("failed to create debug messender!");
			}
		}
	}

	void DestroyDebugMessengerExt() {
		if (kEnableValidationLayers) {
			auto DestroyDebugUtilsMessengerExtFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkCreateDebugUtilsMessengerEXT");
			if (DestroyDebugUtilsMessengerExtFunc) {
				DestroyDebugUtilsMessengerExtFunc(vk_instance, vk_debug_messenger, nullptr);
			}
		}
	}

	void CreateSurface() {
		if (glfwCreateWindowSurface(vk_instance, window, nullptr, &vk_surface) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create window surface!");
		}
	}

	void PickPhysicalDevice() {
		uint32_t physical_device_cnt = 0; 
		vkEnumeratePhysicalDevices(vk_instance, &physical_device_cnt, nullptr);
		if (physical_device_cnt == 0) {
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> physical_devices(physical_device_cnt);
		vkEnumeratePhysicalDevices(vk_instance, &physical_device_cnt, physical_devices.data());
		for (const auto& pdevice : physical_devices) {
			if (CheckPhysicalDeviceAdequate(pdevice)) {
				vk_physical_device = pdevice;
				break;
			}
		}

		if (vk_physical_device == VK_NULL_HANDLE) {
			throw std::runtime_error("Failed to find an adequate GPU!");
		}
	}

	void CreateLogicalDevice() {
		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		std::set<uint32_t> unique_queue_families = {vk_queue_family_index.graphics_family.value(), vk_queue_family_index.present_family.value()};
		float queue_priority = 1.0f;

		for (uint32_t queue_fam_idx : unique_queue_families) {
			VkDeviceQueueCreateInfo queue_create_info = {};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = queue_fam_idx;
			queue_create_info.queueCount = 1;
			queue_create_info.pQueuePriorities = &queue_priority;
			queue_create_infos.push_back(queue_create_info);
		}

		VkPhysicalDeviceFeatures physical_device_features = {};
		VkDeviceCreateInfo logical_device_create_info = {};
		{
			logical_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			logical_device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
			logical_device_create_info.pQueueCreateInfos = queue_create_infos.data();
			logical_device_create_info.pEnabledFeatures = &physical_device_features;
			logical_device_create_info.enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size());
			logical_device_create_info.ppEnabledExtensionNames = kDeviceExtensions.data();
			if (kEnableValidationLayers) {
				logical_device_create_info.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
				logical_device_create_info.ppEnabledLayerNames = kValidationLayers.data();
			} else {
				logical_device_create_info.enabledLayerCount = 0;
			}
		}

		if (vkCreateDevice(vk_physical_device, &logical_device_create_info, nullptr, &vk_logical_device) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create logical device!");
		}
		vkGetDeviceQueue(vk_logical_device, vk_queue_family_index.graphics_family.value(), 0, &vk_graphics_queue);
		vkGetDeviceQueue(vk_logical_device, vk_queue_family_index.present_family.value(), 0, &vk_present_queue);
	}

	void CreateSwapChain() {
		auto ChooseSwapchainSurfaceFormat = [this]() {
			if (vk_swapchain_support_details.formats.size() == 1 && vk_swapchain_support_details.formats[0].format == VK_FORMAT_UNDEFINED) {
				return VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
			}
			
			for (const auto& format : vk_swapchain_support_details.formats) {
				if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					return format;
				}
			}
			return vk_swapchain_support_details.formats[0];
		};

		auto ChooseSwapchainPresentMode = [this]() {
			VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
			for (const auto& present_mode : vk_swapchain_support_details.present_modes) {
				if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
					return present_mode;
				} else if (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
					mode = present_mode;
				}
			}
			return mode;
		};

		auto ChooseSwapchianExtent = [this]() {
			if (vk_swapchain_support_details.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
				return vk_swapchain_support_details.capabilities.currentExtent;
			} else {
				VkExtent2D extent = {width, height};
				extent.width = std::max(vk_swapchain_support_details.capabilities.minImageExtent.width, extent.width);
				extent.height = std::max(vk_swapchain_support_details.capabilities.minImageExtent.height, extent.height);
				return extent;
			}
		};
		VkSurfaceFormatKHR surface_format = ChooseSwapchainSurfaceFormat();
		VkPresentModeKHR present_mode = ChooseSwapchainPresentMode();
		VkExtent2D extent = ChooseSwapchianExtent();

		uint32_t image_count = vk_swapchain_support_details.capabilities.minImageCount + 1;
		if (vk_swapchain_support_details.capabilities.maxImageCount > 0 && image_count > vk_swapchain_support_details.capabilities.maxImageCount) {
			image_count = vk_swapchain_support_details.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR swapchain_create_info = {};
		swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchain_create_info.surface = vk_surface;
		swapchain_create_info.minImageCount = image_count;
		swapchain_create_info.imageFormat = surface_format.format;
		swapchain_create_info.imageColorSpace = surface_format.colorSpace;
		swapchain_create_info.imageExtent = extent;
		swapchain_create_info.imageArrayLayers = 1;
		swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t queue_fam_indices[] = {vk_queue_family_index.graphics_family.value(), vk_queue_family_index.present_family.value()};
		if (vk_queue_family_index.graphics_family != vk_queue_family_index.present_family) {
			swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapchain_create_info.queueFamilyIndexCount = 2;
			swapchain_create_info.pQueueFamilyIndices = queue_fam_indices;
		} else {
			swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		swapchain_create_info.preTransform = vk_swapchain_support_details.capabilities.currentTransform;
		swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchain_create_info.presentMode = present_mode;
		swapchain_create_info.clipped = VK_TRUE;
		swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(vk_logical_device, &swapchain_create_info, nullptr, &vk_swapchain) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create swap chain!");
		}

		uint32_t swapchain_image_count = 0;
		vkGetSwapchainImagesKHR(vk_logical_device, vk_swapchain, &swapchain_image_count, nullptr);
		vk_swapchain_images.resize(swapchain_image_count);
		vkGetSwapchainImagesKHR(vk_logical_device, vk_swapchain, &swapchain_image_count, vk_swapchain_images.data());

		vk_swapchain_image_format = surface_format.format;
		vk_swapchain_image_extent = extent;

		vk_swapchain_image_views.resize(swapchain_image_count);
		for (size_t i = 0; i < swapchain_image_count; ++i) {
			VkImageViewCreateInfo image_view_create_info = {};
			image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			image_view_create_info.image = vk_swapchain_images[i];
			image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			image_view_create_info.format = surface_format.format;
			image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			image_view_create_info.subresourceRange.baseMipLevel = 0;
			image_view_create_info.subresourceRange.levelCount = 1;
			image_view_create_info.subresourceRange.baseArrayLayer = 0;
			image_view_create_info.subresourceRange.layerCount = 1;

			if (vkCreateImageView(vk_logical_device, &image_view_create_info, nullptr, &vk_swapchain_image_views[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create image view!");
			}
		}
	}

	void CreateRenderPass() {
		VkAttachmentDescription color_attachment = {};
		{
			color_attachment.format = vk_swapchain_image_format;
			color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}
		VkAttachmentReference color_attach_ref = {};
		{
			color_attach_ref.attachment = 0;
			color_attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
		VkSubpassDescription subpass_desc = {};
		{
			subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass_desc.colorAttachmentCount = 1;
			subpass_desc.pColorAttachments = &color_attach_ref;
		}
		VkSubpassDependency subpass_dependency = {};
		{
			subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			subpass_dependency.dstSubpass = 0;
			subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			subpass_dependency.srcAccessMask = 0;
			subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		}

		VkRenderPassCreateInfo render_pass_create_info = {};
		{
			render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			render_pass_create_info.attachmentCount = 1;
			render_pass_create_info.pAttachments = &color_attachment;
			render_pass_create_info.subpassCount = 1;
			render_pass_create_info.pSubpasses = &subpass_desc;
			render_pass_create_info.dependencyCount = 1;
			render_pass_create_info.pDependencies = &subpass_dependency;
		}
		if (vkCreateRenderPass(vk_logical_device, &render_pass_create_info, nullptr, &vk_render_pass) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create render pass!");
		}
	}

	void CreateGraphicsPileline() {
		// shader
		std::vector<char> vert_shader_code = ReadFile("shaders/vert.spv");
		std::vector<char> frag_shader_code = ReadFile("shaders/frag.spv");
		auto CreateShaderModule = [this](const std::vector<char>& code) {
			VkShaderModuleCreateInfo shader_module_create_info = {};
			shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shader_module_create_info.codeSize = code.size();
			shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

			VkShaderModule shader;
			if (vkCreateShaderModule(vk_logical_device, &shader_module_create_info, nullptr, &shader) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create shader module!");
			}
			return shader;
		};
		VkShaderModule vert_shader_module = CreateShaderModule(vert_shader_code);
		VkShaderModule frag_shader_module = CreateShaderModule(frag_shader_code);

		VkPipelineShaderStageCreateInfo pl_vert_shader_stage_create_info = {};
		pl_vert_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pl_vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		pl_vert_shader_stage_create_info.module = vert_shader_module;
		pl_vert_shader_stage_create_info.pName = "main";
		VkPipelineShaderStageCreateInfo pl_frag_shader_stage_create_info = {};
		pl_frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pl_frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		pl_frag_shader_stage_create_info.module = frag_shader_module;
		pl_frag_shader_stage_create_info.pName = "main";
		VkPipelineShaderStageCreateInfo pl_shader_stage_create_infos[] = { pl_vert_shader_stage_create_info, pl_frag_shader_stage_create_info };
		// input vertex
		VkPipelineVertexInputStateCreateInfo pl_vertexinput_stage_create_info = {};
		pl_vertexinput_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		pl_vertexinput_stage_create_info.vertexBindingDescriptionCount = 0;
		pl_vertexinput_stage_create_info.vertexAttributeDescriptionCount = 0;
		// input assembly
		VkPipelineInputAssemblyStateCreateInfo pl_inputassembly_stage_create_info = {};
		pl_inputassembly_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		pl_inputassembly_stage_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pl_inputassembly_stage_create_info.primitiveRestartEnable = VK_FALSE;
		// view port 
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)vk_swapchain_image_extent.width;
		viewport.height = (float)vk_swapchain_image_extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = {0, 0};
		scissor.extent = vk_swapchain_image_extent;

		VkPipelineViewportStateCreateInfo pl_viewport_stage_create_info = {};
		pl_viewport_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		pl_viewport_stage_create_info.viewportCount = 1;
		pl_viewport_stage_create_info.pViewports = &viewport;
		pl_viewport_stage_create_info.scissorCount = 1;
		pl_viewport_stage_create_info.pScissors = &scissor;
		// rasterization
		VkPipelineRasterizationStateCreateInfo pl_rasterization_stage_create_info = {};
		pl_rasterization_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		pl_rasterization_stage_create_info.depthClampEnable = VK_FALSE;
		pl_rasterization_stage_create_info.rasterizerDiscardEnable = VK_FALSE;
		pl_rasterization_stage_create_info.polygonMode = VK_POLYGON_MODE_FILL;
		pl_rasterization_stage_create_info.lineWidth = 1.0f;
		pl_rasterization_stage_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
		pl_rasterization_stage_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
		pl_rasterization_stage_create_info.depthBiasEnable = VK_FALSE;

		// multisample
		VkPipelineMultisampleStateCreateInfo pl_multisample_stage_create_info = {};
		pl_multisample_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		pl_multisample_stage_create_info.sampleShadingEnable = VK_FALSE;
		pl_multisample_stage_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// blend 
		VkPipelineColorBlendAttachmentState color_blend_attachment = {};
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo pl_colorblend_state_create_info = {};
		pl_colorblend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		pl_colorblend_state_create_info.logicOpEnable = VK_FALSE;
		pl_colorblend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
		pl_colorblend_state_create_info.attachmentCount = 1;
		pl_colorblend_state_create_info.pAttachments = &color_blend_attachment;
		pl_colorblend_state_create_info.blendConstants[0] = 0.0f;
		pl_colorblend_state_create_info.blendConstants[1] = 0.0f;
		pl_colorblend_state_create_info.blendConstants[2] = 0.0f;
		pl_colorblend_state_create_info.blendConstants[3] = 0.0f;

		// pipeline layout
		VkPipelineLayoutCreateInfo pl_layout_create_info = {};
		pl_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pl_layout_create_info.setLayoutCount = 0;
		pl_layout_create_info.pushConstantRangeCount = 0;
		if (vkCreatePipelineLayout(vk_logical_device, &pl_layout_create_info, nullptr, &vk_pipeline_layout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create pipeline layout!");
		}

		// graphics pipeline 
		VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
		graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphics_pipeline_create_info.stageCount = 2;
		graphics_pipeline_create_info.pStages = pl_shader_stage_create_infos;
		graphics_pipeline_create_info.pVertexInputState = &pl_vertexinput_stage_create_info;
		graphics_pipeline_create_info.pInputAssemblyState = &pl_inputassembly_stage_create_info;
		graphics_pipeline_create_info.pViewportState = &pl_viewport_stage_create_info;
		graphics_pipeline_create_info.pRasterizationState = &pl_rasterization_stage_create_info;
		graphics_pipeline_create_info.pMultisampleState = &pl_multisample_stage_create_info;
		graphics_pipeline_create_info.pColorBlendState = &pl_colorblend_state_create_info;
		graphics_pipeline_create_info.layout = vk_pipeline_layout;
		graphics_pipeline_create_info.renderPass = vk_render_pass;
		graphics_pipeline_create_info.subpass = 0;
		graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(vk_logical_device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, &vk_pipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create graphics pipeline~");
		}
		vkDestroyShaderModule(vk_logical_device, frag_shader_module, nullptr);
		vkDestroyShaderModule(vk_logical_device, vert_shader_module, nullptr);
	}

	void CreateFramebuffers() {
		vk_framebuffers.resize(vk_swapchain_image_views.size());
		for (size_t i = 0; i < vk_swapchain_image_views.size(); ++i) {
			VkImageView attachments[] = { vk_swapchain_image_views[i] };
			VkFramebufferCreateInfo framebuffer_create_info = {};
			framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_create_info.renderPass = vk_render_pass;
			framebuffer_create_info.attachmentCount = 1;
			framebuffer_create_info.pAttachments = attachments;
			framebuffer_create_info.width = vk_swapchain_image_extent.width;
			framebuffer_create_info.height = vk_swapchain_image_extent.height;
			framebuffer_create_info.layers = 1;

			if (vkCreateFramebuffer(vk_logical_device, &framebuffer_create_info, nullptr, &vk_framebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create famebuffer!");
			}
		}
	}

	void CreateCommandPool() {
		VkCommandPoolCreateInfo command_pool_create_info = {};
		command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		command_pool_create_info.queueFamilyIndex = vk_queue_family_index.graphics_family.value();
		if (vkCreateCommandPool(vk_logical_device, &command_pool_create_info, nullptr, &vk_command_pool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create command pool!");
		}
	}

	void CreateCommandBuffers() {
		vk_command_buffers.resize(vk_framebuffers.size());
		VkCommandBufferAllocateInfo command_buffer_alloc_info = {};
		command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command_buffer_alloc_info.commandPool = vk_command_pool;
		command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		command_buffer_alloc_info.commandBufferCount = static_cast<uint32_t>(vk_command_buffers.size());
		if (vkAllocateCommandBuffers(vk_logical_device, &command_buffer_alloc_info, vk_command_buffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate Command buffers!");
		}

		for (size_t i = 0; i < vk_command_buffers.size(); ++i) {
			VkCommandBufferBeginInfo cb_begin_info = {};
			cb_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cb_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			if (vkBeginCommandBuffer(vk_command_buffers[i], &cb_begin_info) != VK_SUCCESS) {
				throw std::runtime_error("Failed to begin recording command buffer!");
			}

			VkRenderPassBeginInfo rp_begin_info = {};
			rp_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			rp_begin_info.renderPass = vk_render_pass;
			rp_begin_info.framebuffer = vk_framebuffers[i];
			rp_begin_info.renderArea.offset = { 0, 0 };
			rp_begin_info.renderArea.extent = vk_swapchain_image_extent;
			VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
			rp_begin_info.clearValueCount = 1;
			rp_begin_info.pClearValues = &clear_color;

			vkCmdBeginRenderPass(vk_command_buffers[i], &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(vk_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline);
			vkCmdDraw(vk_command_buffers[i], 3, 1, 0, 0);
			vkCmdEndRenderPass(vk_command_buffers[i]);
			if (vkEndCommandBuffer(vk_command_buffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to recode commad buffer!");
			}
		}
	}

	void CreateSyncObjects() {
		vk_image_available_semaphores.resize(kMaxFramesInFlight);
		vk_reder_finished_semaphores.resize(kMaxFramesInFlight);
		vk_fences.resize(kMaxFramesInFlight);

		VkSemaphoreCreateInfo semaphore_create_info = {};
		semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fence_create_info = {};
		fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
			if (vkCreateSemaphore(vk_logical_device, &semaphore_create_info, nullptr, &vk_image_available_semaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(vk_logical_device, &semaphore_create_info, nullptr, &vk_reder_finished_semaphores[i]) != VK_SUCCESS ||
				vkCreateFence(vk_logical_device, &fence_create_info, nullptr, &vk_fences[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create synchronization objects for a frame!");
			}
		}
	}

	void DrawFrame() {
		vkWaitForFences(vk_logical_device, 1, &vk_fences[vk_current_frame], VK_TRUE, std::numeric_limits<uint64_t>::max());
		vkResetFences(vk_logical_device, 1, &vk_fences[vk_current_frame]);

		uint32_t image_index;
		vkAcquireNextImageKHR(vk_logical_device, vk_swapchain, std::numeric_limits<uint64_t>::max(), 
				vk_image_available_semaphores[vk_current_frame], VK_NULL_HANDLE, &image_index);
		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		
		VkSemaphore wait_semaphores[] = { vk_image_available_semaphores[vk_current_frame] };
		VkSemaphore signal_semaphores[] = { vk_reder_finished_semaphores[vk_current_frame] };
		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = wait_semaphores;
		submit_info.pWaitDstStageMask = wait_stages;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &vk_command_buffers[image_index];
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = signal_semaphores;

		if (vkQueueSubmit(vk_graphics_queue, 1, &submit_info, vk_fences[vk_current_frame]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit draw command buffer!");
		}

		VkPresentInfoKHR present_info = {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = signal_semaphores;
		VkSwapchainKHR swapchains[] = {vk_swapchain};
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swapchains;
		present_info.pImageIndices = &image_index;
		vkQueuePresentKHR(vk_present_queue, &present_info);

		vk_current_frame = (vk_current_frame + 1) % kMaxFramesInFlight;
	}
private:
	int width;
	int height;
	GLFWwindow* window;
	VkInstance vk_instance;
	VkDebugUtilsMessengerEXT vk_debug_messenger;
	VkSurfaceKHR vk_surface;
	VkPhysicalDevice vk_physical_device{VK_NULL_HANDLE};
	QueueFamilyIndex vk_queue_family_index;
	SwapChainSupportDetails vk_swapchain_support_details;
	VkDevice vk_logical_device;

	VkQueue vk_graphics_queue;
	VkQueue vk_present_queue;

	VkSwapchainKHR vk_swapchain;
	std::vector<VkImage> vk_swapchain_images;
	VkFormat vk_swapchain_image_format;
	VkExtent2D vk_swapchain_image_extent;
	std::vector<VkImageView> vk_swapchain_image_views;

	VkRenderPass vk_render_pass;
	VkPipelineLayout vk_pipeline_layout;
	VkPipeline vk_pipeline;

	std::vector<VkFramebuffer> vk_framebuffers;
	VkCommandPool vk_command_pool;
	std::vector<VkCommandBuffer> vk_command_buffers;

	std::vector<VkSemaphore> vk_image_available_semaphores;
	std::vector<VkSemaphore> vk_reder_finished_semaphores;
	std::vector<VkFence> vk_fences;

	size_t vk_current_frame{ 0 };
};

int main() {
	VkDRender render(800, 600);
	try {
		render.Run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}