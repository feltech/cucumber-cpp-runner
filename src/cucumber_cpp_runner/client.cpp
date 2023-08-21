// Copyright 2023 David Feltell
// SPDX-License-Identifier: MIT
#include "./client.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>

#include <boost/process.hpp>
#include <fmt/format.h>

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

	// Cucumber-Wire doesn't have any documentation on how it locates .wire config files, but based
	// on a glance at the code and experimentation, the cwd must have a `features` dir in it, which
	// will then be searched recursively.
	//
	// So assume if a non-directory is given, then it's a `.feature` file inside a `features`
	// directory. A bit brittle, but works OK.
	fs::path const feature_dir =
		fs::is_directory(feature_path) ? feature_path : feature_path.parent_path().parent_path();

	if (verbose)
		fmt::print(stderr, "Executing '{}' in '{}'\n", cucumber_cmd, feature_dir.string());

	bp::child cucumber = [&cucumber_cmd, &feature_dir, &cucumber_stdout]
	{
		auto env = boost::this_process::environment();
		for (auto const & entry : env) fmt::print(stderr, entry.to_string());
		// Silence "THIS RUBY IMPLEMENTATION DOESN'T REPORT FILE AND LINE FOR PROCS"
		env["RUBY_IGNORE_CALLERS"] = "1";

		return bp::child{
			cucumber_cmd.c_str(),
			bp::std_in.close(),
			(bp::std_out & bp::std_err) > cucumber_stdout,
			env,
			bp::start_dir(feature_dir.string())};
	}();

	cucumber.wait();

	int const exit_code = cucumber.exit_code();

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
	if (fs::is_regular_file(cucumber_exe) || fs::is_symlink(cucumber_exe))
		return cucumber_exe;

	// Grr, libstdc++ 12.2/13.1 UBSan-detected bug:
	//   https://gcc.gnu.org/bugzilla//show_bug.cgi?id=109703
	fs::path resolved_path = boost::process::search_path(cucumber_exe);
	if (resolved_path.empty())
		throw std::runtime_error{
			fmt::format("Cucumber executable not found using '{}'", cucumber_exe.string())};
	return resolved_path;
}

}  // namespace v1
}  // namespace cucumber_cpp_runner