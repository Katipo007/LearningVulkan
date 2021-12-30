#pragma once

#include <span>
#include <string_view>

class App
{
public:
	virtual ~App() = default;

	void Run(std::span<std::string_view> args)
	{
		OnInit( args );
		MainLoop();
		OnDeinit();
	}

protected:
	virtual void OnInit(std::span<std::string_view> args) = 0;
	virtual void MainLoop() = 0;
	virtual void OnDeinit() = 0;
};
