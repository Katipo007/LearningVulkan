
#include <cassert>
#include <cinttypes>
#include <iostream>
#include <string_view>
#include <vector>

#include "App.hpp"
#include "Application/TriangleApp.hpp"

int main([[maybe_unused]] const int argc, [[maybe_unused]] const char** argv)
{
	std::vector<std::string_view> command_line_args(static_cast<std::size_t>(argc));
	for (int i = 0; i < argc; i++) {
		command_line_args.emplace_back(argv[i]);
	}

	std::unique_ptr<App> app;

	app = std::make_unique<TriangleApp>();

	assert(app != nullptr);

	try
	{
		app->Run(command_line_args);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Unhandled exception while running application: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	app.reset();

	return EXIT_SUCCESS;
}
