#pragma once

#include <functional>

class Defer {
private:
	std::function<void()> _func;
public:
	Defer(std::function<void()>func);
	~Defer();
};

