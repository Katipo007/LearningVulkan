
#include <iostream>

#include "LearningVulkan/LibraryWrappers/GLFW.hpp"
#include "LearningVulkan/LibraryWrappers/glm.hpp"
#include "LearningVulkan/LibraryWrappers/vulkan.hpp"

#include "TriangleApp.hpp"

using GLFWWindowHandle = std::unique_ptr<GLFWwindow, decltype([](GLFWwindow* window) { glfwDestroyWindow(window); })>;

struct TriangleApp::Pimpl
{
	GLFWWindowHandle window{};
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
	pimpl->window = GLFWWindowHandle{ glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr) };

	uint32_t extension_count{ 0 };
	if (const auto e = vk::enumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr); e != vk::Result::eSuccess)
	{
		std::cerr << "Error result from vkEnumerateInstanceExtensionProperties: " << e << '\n';
	}

	std::cout << extension_count << " extensions supported\n";
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
	glfwTerminate();
}
