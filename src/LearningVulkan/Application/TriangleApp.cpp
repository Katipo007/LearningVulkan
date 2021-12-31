
#include <algorithm>
#include <iostream>
#include <limits>
#include <optional>
#include <tuple>
#include <set>
#include <string_view>
#include <vector>

#include "LearningVulkan/Bridges/GLFW.hpp"
#include "LearningVulkan/Bridges/glm.hpp"
#include "LearningVulkan/Bridges/vulkan.hpp"

#include "TriangleApp.hpp"

using GLFWWindowHandle = std::unique_ptr<GLFWwindow, decltype([](GLFWwindow* window) { glfwDestroyWindow(window); })>;

namespace TriangleApp_NS
{
	using namespace std::string_view_literals;

	constexpr glm::ivec2 window_size{ 800, 600 };
	constexpr std::string_view window_title{ "Vulkan window" };

	constexpr std::array required_device_extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	constexpr std::array validation_layers{ "VK_LAYER_KHRONOS_validation" };
#ifdef _DEBUG
	constexpr bool use_validation_layers{ true };
#else
	constexpr bool use_validation_layers{ false };
#endif

	static VKAPI_ATTR VkBool32 VKAPI_CALL OnVulkanDebugCallback(
		[[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
		[[maybe_unused]] void* user_data)
	{
		std::cerr << "[Validation-layer]"
			<< '[' << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagBitsEXT>(type)) << ']'
			<< '[' << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(severity)) << ']'
			<< ' ' << callback_data->pMessage
			<< '\n';

		return VK_FALSE;
	}

	[[nodiscard]] static bool CheckValidationLayerSupport()
	{
		const auto layers = vk::enumerateInstanceLayerProperties();
		for (const std::string_view layer_name : validation_layers)
		{
			const auto it = std::find_if(std::begin(layers), std::end(layers), [layer_name](const vk::LayerProperties& layer) { return layer.layerName == layer_name; });
			if (it == std::end(layers))
			{
				std::cout << "Missing support for validation layer with name '" << layer_name << "'\n";
				return false;
			}
		}

		return true;
	}

	[[nodiscard]] static std::vector<const char*> GetRequiredExtensions()
	{
		uint32_t glfw_extension_count{ 0 };
		const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

		if (glfw_extensions == NULL)
		{
			const char* msg{};
			const auto error = glfwGetError(&msg);
			throw std::runtime_error("GLFW reports it does not support vulkan. glfwGetError code=" + std::to_string(error) + ", message='" + msg + "'");
		}

		std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

		if (use_validation_layers) {
			extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	[[nodiscard]] static bool CheckDeviceExtensionSupport(const vk::PhysicalDevice& device)
	{
		const auto available_extensions{ device.enumerateDeviceExtensionProperties() };

		// using this as a "checklist" of extensions still needing to be found.
		std::set<std::string_view> required_extensions{ std::begin(required_device_extensions), std::end(required_device_extensions) };

		for (const auto& extension : available_extensions) {
			required_extensions.erase(std::string_view{ extension.extensionName });
		}

		return required_extensions.empty();
	}

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphics_family{};
		std::optional<uint32_t> present_family{};

		bool IsComplete() const noexcept
		{
			return graphics_family.has_value()
				&& present_family.has_value();
		}
	};

	struct SwapChainSupportDetails
	{
		SwapChainSupportDetails() noexcept = default;
		[[nodiscard]] SwapChainSupportDetails(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface)
			: capabilities{ device.getSurfaceCapabilitiesKHR(surface)}
			, formats{ device.getSurfaceFormatsKHR(surface)}
			, present_modes{ device.getSurfacePresentModesKHR(surface)}
		{}

		vk::SurfaceCapabilitiesKHR capabilities{};
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> present_modes;
	};

	[[nodiscard]] QueueFamilyIndices FindQueueFamilies(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface)
	{
		QueueFamilyIndices indices{};

		const auto queue_families = device.getQueueFamilyProperties();
		for (uint32_t idx{0}; const auto & properties : queue_families)
		{
			if (properties.queueFlags & vk::QueueFlagBits::eGraphics) {
				indices.graphics_family = idx;
			}

			if (device.getSurfaceSupportKHR(idx, surface)) {
				indices.present_family = idx;
			}

			if (indices.IsComplete()) {
				break;
			}
			++idx;
		}

		return indices;
	}

	[[nodiscard]] vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::span<const vk::SurfaceFormatKHR> available_formats)
	{
		for (auto& format : available_formats)
		{
			if (format.format == vk::Format::eB8G8R8A8Srgb
				&& format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear )
			{
				return format;
			}
		}

		// fallback to the first entry
		return available_formats.front();
	}

	[[nodiscard]] vk::PresentModeKHR ChoosePresentMode(const std::span<const vk::PresentModeKHR> available_modes)
	{
		// We prefer some modes over others
		if (const auto it = std::find(std::begin(available_modes), std::end(available_modes), vk::PresentModeKHR::eMailbox); it != std::end(available_modes)) {
			return *it;
		}

		return vk::PresentModeKHR::eFifo; // FIFO is guaranteed to be available
	}

	[[nodiscard]] vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window = nullptr)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else if( window )
		{
			int window_w{}, window_h{};
			glfwGetWindowSize(window, &window_w, &window_h);
			return {
				std::clamp(static_cast<uint32_t>(window_w), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
				std::clamp(static_cast<uint32_t>(window_h), capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
			};
		}
		else
		{
			assert(false && "Unideal case");
			return vk::Extent2D{ static_cast<uint32_t>(window_size.x), static_cast<uint32_t>(window_size.y) };
		}
	}

	[[nodiscard]] static bool IsSuitableDevice(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface)
	{
		[[maybe_unused]] const auto& properties{ device.getProperties() };
		[[maybe_unused]] const auto& features{ device.getFeatures() };
		const auto family_indices{ FindQueueFamilies(device, surface) };

		const bool supports_required_extensions{ CheckDeviceExtensionSupport(device) };
		const bool swap_chain_adequite = [&](){
			if (supports_required_extensions) {
				const SwapChainSupportDetails swap_chain_details{ device, surface };
				return !swap_chain_details.formats.empty() && !swap_chain_details.present_modes.empty();
			}
			return false;
		}();

		// example: only accept dedicated devices which support geometry shaders.
		//return properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu && features.geometryShader;

		// could also rank all the devices and pick the best one by default.

		return family_indices.IsComplete()
			&& supports_required_extensions
			&& swap_chain_adequite;
	}

	[[nodiscard]] static vk::UniqueInstance CreateInstance()
	{
		if (use_validation_layers && !CheckValidationLayerSupport()) {
			throw std::runtime_error("At least one of the validation layers requested is not available");
		}

		const vk::ApplicationInfo app_info
		{
			"Triangle", VK_MAKE_API_VERSION(0, 1, 0, 0),
			"No Engine", VK_MAKE_API_VERSION(0, 1, 0, 0),
			VK_API_VERSION_1_2
		};

		const auto required_extensions{ GetRequiredExtensions() };

		vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_info
		{
			{},
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
			TriangleApp_NS::OnVulkanDebugCallback
		};

		vk::InstanceCreateInfo create_info{};
		create_info.setPApplicationInfo(&app_info);
		create_info.setPEnabledExtensionNames(required_extensions);
		if (use_validation_layers) {
			create_info.setPEnabledLayerNames(validation_layers);
			assert(create_info.pNext == nullptr);
			create_info.setPNext(&debug_messenger_info);
		}

		return vk::createInstanceUnique(create_info);
	}

	[[nodiscard]] vk::UniqueSwapchainKHR CreateSwapChain(vk::Device& device, const vk::PhysicalDevice& physical_device, const vk::SurfaceKHR& surface, GLFWwindow* window = nullptr)
	{
		SwapChainSupportDetails swap_chain_support_details{ physical_device, surface };
		const auto surface_format{ ChooseSwapSurfaceFormat(swap_chain_support_details.formats) };
		const auto present_mode{ ChoosePresentMode(swap_chain_support_details.present_modes) };
		const auto extent{ ChooseSwapExtent(swap_chain_support_details.capabilities, window) };
		const uint32_t max_supported_images{ (swap_chain_support_details.capabilities.maxImageCount > 0) ? swap_chain_support_details.capabilities.maxImageCount : std::numeric_limits<uint32_t>::max() };
		const auto indices = FindQueueFamilies(physical_device, surface);
		const auto family_indices = std::array{ indices.graphics_family.value(), indices.present_family.value() };

		vk::SwapchainCreateInfoKHR create_info{};
		create_info.setSurface(surface);
		create_info.setMinImageCount(std::min(swap_chain_support_details.capabilities.minImageCount + 1, max_supported_images));
		create_info.setImageFormat(surface_format.format);
		create_info.setImageColorSpace(surface_format.colorSpace);
		create_info.setImageExtent(extent);
		create_info.setImageArrayLayers(1);
		create_info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
		if (indices.graphics_family.value() != indices.present_family.value())
		{
			create_info.setImageSharingMode(vk::SharingMode::eConcurrent);
			create_info.setQueueFamilyIndices(family_indices);
		}
		else
		{
			create_info.setImageSharingMode(vk::SharingMode::eExclusive);
			create_info.setQueueFamilyIndices({}); // Optional
		}
		create_info.setPreTransform(swap_chain_support_details.capabilities.currentTransform);
		create_info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
		create_info.setPresentMode(present_mode);
		create_info.setClipped(VK_TRUE);
		create_info.setOldSwapchain(VK_NULL_HANDLE); // Previous swap chain which became invalidated (e.g. via window being resized). TODO

		return device.createSwapchainKHRUnique(create_info);
	}

	[[nodiscard]] static std::tuple<vk::UniqueDevice, vk::PhysicalDevice, QueueFamilyIndices> CreateDevice(vk::Instance& instance, const vk::SurfaceKHR& surface)
	{
		const auto physical_devices = instance.enumeratePhysicalDevices();
		if (physical_devices.empty()) {
			throw std::runtime_error("No physical devices available");
		}

		const auto best_device = std::find_if(std::begin(physical_devices), std::end(physical_devices), std::bind(TriangleApp_NS::IsSuitableDevice, std::placeholders::_1, surface));
		if (best_device == std::end(physical_devices)) {
			throw std::runtime_error("No suitable physical devices available");
		}

		const auto indices{ FindQueueFamilies(*best_device, surface) };
		assert(indices.IsComplete());

		const float priority{ 1.f };
		std::vector<vk::DeviceQueueCreateInfo> queues_info;
		const std::set<uint32_t> unique_families{ indices.graphics_family.value(), indices.present_family.value() };
		for (uint32_t family : unique_families) {
			queues_info.emplace_back(vk::DeviceQueueCreateFlags{}, family, 1U, &priority);
		}

		vk::PhysicalDeviceFeatures features{};

		vk::DeviceCreateInfo create_info{};
		create_info.setPEnabledExtensionNames(required_device_extensions);
		create_info.setPEnabledFeatures(&features);
		create_info.setQueueCreateInfos(queues_info);
		if (use_validation_layers) {
			create_info.setPEnabledLayerNames(validation_layers);
		}

		return { best_device->createDeviceUnique(create_info), *best_device, indices };
	}
}

struct TriangleApp::Pimpl
{
	GLFWWindowHandle window{};
	vk::UniqueInstance vk_instance{};
	vk::SurfaceKHR surface{};
	vk::UniqueDevice vk_device{};
	vk::Queue graphics_queue{};
	vk::Queue present_queue{};
	vk::UniqueSwapchainKHR swap_chain{};
	std::vector<vk::Image> swap_chain_images{};
};

TriangleApp::TriangleApp()
	: pimpl{ std::make_unique<Pimpl>() }
{
}

TriangleApp::~TriangleApp() = default;

void TriangleApp::OnInit([[maybe_unused]] std::span<std::string_view> cli)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	pimpl->window = GLFWWindowHandle{ glfwCreateWindow(TriangleApp_NS::window_size.x, TriangleApp_NS::window_size.y, TriangleApp_NS::window_title.data(), nullptr, nullptr) };

	pimpl->vk_instance = TriangleApp_NS::CreateInstance();
	VkSurfaceKHR surface{};
	if (const auto result = glfwCreateWindowSurface(*pimpl->vk_instance, pimpl->window.get(), {}, &surface); result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface");
	}
	else {
		pimpl->surface = surface;
	}

	TriangleApp_NS::QueueFamilyIndices indices{};
	vk::PhysicalDevice physical_device{};
	std::tie(pimpl->vk_device, physical_device, indices) = TriangleApp_NS::CreateDevice(*pimpl->vk_instance, surface);
	assert(pimpl->vk_device);
	assert(indices.IsComplete());

	pimpl->graphics_queue = pimpl->vk_device->getQueue(indices.graphics_family.value(), 0);
	pimpl->present_queue = pimpl->vk_device->getQueue(indices.present_family.value(), 0);

	pimpl->swap_chain = TriangleApp_NS::CreateSwapChain(*pimpl->vk_device, physical_device, pimpl->surface, pimpl->window.get());
	pimpl->swap_chain_images = pimpl->vk_device->getSwapchainImagesKHR(*pimpl->swap_chain);
}

void TriangleApp::MainLoop()
{
	while (!glfwWindowShouldClose(pimpl->window.get()))
	{
		glfwPollEvents();
	}
}

void TriangleApp::OnDeinit()
{
	pimpl->swap_chain_images.clear();
	pimpl->swap_chain.reset();
	pimpl->present_queue = VK_NULL_HANDLE;
	pimpl->graphics_queue = VK_NULL_HANDLE;
	pimpl->vk_device.reset();
	pimpl->vk_instance->destroySurfaceKHR(pimpl->surface);
	pimpl->surface = VK_NULL_HANDLE;
	pimpl->vk_instance.reset();
	pimpl->window.reset();

	glfwTerminate();
}
