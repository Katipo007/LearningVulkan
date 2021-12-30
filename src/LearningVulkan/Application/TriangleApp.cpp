
#include <algorithm>
#include <iostream>
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

	[[nodiscard]] static vk::UniqueInstance CreateInstance()
	{
		if (use_validation_layers && !CheckValidationLayerSupport()) {
			throw std::runtime_error("At least one of the validation layers requested is not available");
		}

		vk::ApplicationInfo app_info{};
		app_info.sType = vk::StructureType::eApplicationInfo;
		app_info.pApplicationName = "Triangle";
		app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		app_info.pEngineName = "No Engine";
		app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_2;

		uint32_t glfw_extension_count{ 0 };
		const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

		const auto extensions = vk::enumerateInstanceExtensionProperties(nullptr);
#ifdef _DEBUG
		std::cout << "Available extensions:\n";
		for (const auto& extension : extensions)
		{
			std::cout << '\t' << extension.extensionName << '\n';
		}
#endif

		vk::InstanceCreateInfo create_info{};
		create_info.sType = vk::StructureType::eInstanceCreateInfo;
		create_info.pApplicationInfo = &app_info;
		create_info.enabledExtensionCount = glfw_extension_count;
		create_info.ppEnabledExtensionNames = glfw_extensions;
		create_info.enabledLayerCount = 0;

		if (use_validation_layers)
		{
			create_info.setEnabledLayerCount(static_cast<uint32_t>(validation_layers.size()));
			create_info.setPEnabledLayerNames(validation_layers);
		}

		return vk::createInstanceUnique(create_info);
	}
}

struct TriangleApp::Pimpl
{
	GLFWWindowHandle window{};
	vk::UniqueInstance vk_instance;
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
	pimpl->vk_instance.reset();
	pimpl->window.reset();

	glfwTerminate();
}
