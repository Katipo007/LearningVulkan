
#include <cinttypes>
#include <memory>
#include <iostream>

#include "LibraryWrappers/GLFW.hpp"
#include "LibraryWrappers/glm.hpp"
#include "LibraryWrappers/vulkan.hpp"


using GLFWWindowHandle = std::unique_ptr < GLFWwindow, decltype([](GLFWwindow* window) { glfwDestroyWindow(window); }) > ;

int main([[maybe_unused]] const int argc, [[maybe_unused]] const char** argv)
{
	glfwInit();

	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	GLFWWindowHandle window{ glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr) };

	uint32_t extension_count{ 0 };
	if (const auto e = vk::enumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr); e != vk::Result::eSuccess)
	{
		std::cerr << "Error result from vkEnumerateInstanceExtensionProperties: " << e << '\n';
	}

	std::cout << extension_count << " extensions supported\n";

	glm::mat4 matrix{};
	glm::vec4 vec{};
	[[maybe_unused]] auto test = matrix * vec;

	while (!glfwWindowShouldClose(window.get()))
	{
		glfwPollEvents();
	}

	window.reset();

	glfwTerminate();

	return 0;
}
