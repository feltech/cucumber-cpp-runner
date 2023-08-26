// Copyright 2023 David Feltell
// SPDX-License-Identifier: MIT
#include <cstdlib>

#include <fmt/printf.h>

#include <cucumber_cpp_runner/cucumber_cpp_runner.hpp>

int main()
{
	try
	{
		return cucumber_cpp_runner::execute_cucumber_tests({});
	} catch (std::exception const& exc) {
		fmt::print(stderr, "Failed custom main: {}", exc.what());
	}
	return EXIT_FAILURE;
}
