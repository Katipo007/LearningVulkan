#pragma once

#include <memory>

#include "LearningVulkan/App.hpp"

class TriangleApp final
	: public App
{
public:
	TriangleApp();
	~TriangleApp();

protected:
	void OnInit(std::span<std::string_view> cli) override;
	void MainLoop() override;
	void OnDeinit() override;

private:
	struct Pimpl;
	std::unique_ptr<Pimpl> pimpl;
};
