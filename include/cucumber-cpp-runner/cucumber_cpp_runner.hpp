#pragma once
#include <cucumber-cpp-runner/cucumber_cpp_runner_export.hpp>

#include <boost/filesystem/path.hpp>

namespace cucumber_cpp_runner
{
inline namespace v1
{
namespace fs = boost::filesystem;
CUCUMBER_CPP_RUNNER_EXPORT struct CucumberRunnerParams
{
	/// Cucumber command-line executable - absolute or on PATH.
	fs::path cucumber_exe{"cucumber"};
	/// Additional command-line arguments to pass to cucumber executable.
	std::string cucumber_options{};
	/// Location of features directory, or a specific feature file.
	fs::path feature_path{"."};
	/// Enable some verbose CucumberCppRunner-specific logs.
	bool verbose{false};
};

CUCUMBER_CPP_RUNNER_EXPORT int execute_cucumber_tests(const CucumberRunnerParams & params = {});
}  // namespace v1
}  // namespace cucumber_cpp_runner
