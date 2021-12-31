# Learning Vulkan
My intention is to (mostly) follow this tutorial: https://vulkan-tutorial.com/ before expanding upon by myself to try things out and maybe make a small abstraction layer over Vulkan for use in my own C++20 projects.

I'm using:
* Visual Studio 2022
   * C++latest (at least C++20).
   * I'm not using C++20 modules yet but planning to in future.
   * Only testing on Windows 10 currently.
* GLFW
* GLM
* Vulkan SDK v1.2.198.1
   * using the C++ bindings (vulkan/vulkan.hpp rather than .h)

If adapting for your own use you may need to adjust library paths, I've tried to make this easy via the dedicated \*.props files under src\\
