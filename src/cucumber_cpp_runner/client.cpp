// Copyright 2023 David Feltell
// SPDX-License-Identifier: MIT
#include "./client.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>

#include <boost/process.hpp>
#include <fmt/format.h>

// Silence clang-tidy readability-magic-numbers, which doesn't like macros.
constexpr int kExitFailure = EXIT_FAILURE;
constexpr unsigned kCucumberTimeoutMs = 10000;

namespace cucumber_cpp_runner
{
inline namespace v1
{

/**
 * Launch a subprocess to execute `cucumber` command line.
 */

int run_cucumber_exe(
	fs::path const & feature_path,
	std::string const & cucumber_exe,
	std::string const & cucumber_options,
	bool const verbose)
{
	namespace bp = boost::process;

	bp::ipstream cucumber_stdout;

	std::string const cucumber_cmd =
		fmt::format("{} {} {}", cucumber_exe, cucumber_options, feature_path.string());

	if (verbose)
		fmt::print(stderr, "Executing '{}'\n", cucumber_cmd);

	bp::child cucumber = [&cucumber_cmd, &cucumber_stdout]
	{
		auto env = boost::this_process::environment();
		for (auto const & entry : env) fmt::print(stderr, entry.to_string());
		// Silence "THIS RUBY IMPLEMENTATION DOESN'T REPORT FILE AND LINE FOR PROCS"
		env["RUBY_IGNORE_CALLERS"] = "1";

		return bp::child{
			cucumber_cmd.c_str(),
			bp::std_in.close(),
			(bp::std_out & bp::std_err) > cucumber_stdout,
			env};
	}();

	int const exit_code = [&cucumber_cmd, &cucumber]
	{
		if (!cucumber.wait_for(std::chrono::milliseconds{kCucumberTimeoutMs}))
		{
			fmt::print(stderr, "Timeout executing '{}'\n", cucumber_cmd);
			cucumber.terminate();
			return kExitFailure;
		}
		return cucumber.exit_code();
	}();

	std::string const cucumber_output = [&cucumber_stdout]
	{
		std::stringstream sstr;
		sstr << cucumber_stdout.rdbuf();
		return sstr.str();
	}();

	fmt::print("{}\n", cucumber_output);
	return exit_code;
}

fs::path find_cucumber_exe(fs::path const & cucumber_exe)
{
	// Grr, libstdc++ 12.2/13.1 UBSan-detected bug:
	//   https://gcc.gnu.org/bugzilla//show_bug.cgi?id=109703
	fs::path path = boost::process::search_path(cucumber_exe);
	if (path.empty())
		throw std::runtime_error{"'cucumber' executable not found"};
	return path;
}

}  // namespace v1
}  // namespace cucumber_cpp_runner