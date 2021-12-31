
#include <algorithm>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include "LearningVulkan/LibraryWrappers/GLFW.hpp"
#include "LearningVulkan/LibraryWrappers/glm.hpp"
#include "LearningVulkan/LibraryWrappers/vulkan.hpp"

#include "TriangleApp.hpp"

using GLFWWindowHandle = std::unique_ptr<GLFWwindow, decltype([](GLFWwindow* window) { glfwDestroyWindow(window); })>;

namespace TriangleApp_NS
{
	using namespace std::string_view_literals;

	constexpr glm::ivec2 window_size{ 800, 600 };
	constexpr std::string_view window_title{ "Vulkan window" };

	constexpr std::array validation_layers{ "VK_LAYER_KHRONOS_validation" };
#ifdef _DEBUG
	constexpr bool use_validation_layers{ true };
#else
	constexpr bool use_validation_layers{ false };
#endif

	static VKAPI_ATTR VkBool32 VKAPI_CALL OnVulkanDebugCallback(
		[[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		[[maybe_unused]] void* pUserData)
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
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

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphics_family{};

		bool IsComplete() const noexcept
		{
			return graphics_family.has_value();
		}
	};

	[[nodiscard]] QueueFamilyIndices FindQueueFamilies(const vk::PhysicalDevice& device)
	{
		QueueFamilyIndices indices{};

		const auto queue_families = device.getQueueFamilyProperties();
		for (std::size_t idx{0}; const auto & properties : queue_families)
		{
			if (properties.queueFlags & vk::QueueFlagBits::eGraphics) {
				indices.graphics_family = static_cast<uint32_t>(idx);
			}

			if (indices.IsComplete()) {
				break;
			}
			++idx;
		}

		return indices;
	}

	[[nodiscard]] static bool IsSuitableDevice(const vk::PhysicalDevice& device)
	{
		[[maybe_unused]] const auto& properties{ device.getProperties() };
		[[maybe_unused]] const auto& features{ device.getFeatures() };
		const auto family_indices{ FindQueueFamilies(device) };

		// example: only accept dedicated devices which support geometry shaders.
		//return properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu && features.geometryShader;

		// could also rank all the devices and pick the best one by default.

		return family_indices.IsComplete();
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
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
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

	[[nodiscard]] static std::pair<vk::UniqueDevice, QueueFamilyIndices> CreateDevice( vk::Instance& instance )
	{
		const auto physical_devices = instance.enumeratePhysicalDevices();
		if (physical_devices.empty()) {
			throw std::runtime_error("No physical devices available");
		}

		const auto best_device = std::find_if(std::begin(physical_devices), std::end(physical_devices), TriangleApp_NS::IsSuitableDevice);
		if (best_device == std::end(physical_devices)) {
			throw std::runtime_error("No suitable physical devices available");
		}

		vk::DeviceCreateInfo create_info{};

		const auto families{ FindQueueFamilies(*best_device) };
		assert(families.IsComplete());

		const float priority{ 1.f };
		std::vector<vk::DeviceQueueCreateInfo> queues_info;
		queues_info.emplace_back(vk::DeviceQueueCreateFlags{}, families.graphics_family.value(), 1U, &priority);
		create_info.setQueueCreateInfos(queues_info);

		vk::PhysicalDeviceFeatures features{};
		create_info.setPEnabledFeatures(&features);

		if (use_validation_layers) {
			create_info.setPEnabledLayerNames(validation_layers);
		}

		return { best_device->createDeviceUnique(create_info), families };
	}
}

struct TriangleApp::Pimpl
{
	GLFWWindowHandle window{};
	vk::UniqueInstance vk_instance{};
	vk::UniqueDevice vk_device{};
	vk::Queue graphics_queue{};
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
	TriangleApp_NS::QueueFamilyIndices indices{};
	std::tie(pimpl->vk_device, indices) = TriangleApp_NS::CreateDevice(*pimpl->vk_instance);
	assert(pimpl->vk_device);
	assert(indices.IsComplete());

	pimpl->graphics_queue = pimpl->vk_device->getQueue(indices.graphics_family.value(), 0);
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
	pimpl->graphics_queue = nullptr;
	pimpl->vk_device.reset();
	pimpl->vk_instance.reset();
	pimpl->window.reset();

	glfwTerminate();
}
