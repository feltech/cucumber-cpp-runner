#pragma once
#include <cucumber-cpp-runner/cucumber_cpp_runner_export.hpp>

#include <boost/filesystem/path.hpp>

namespace cucumber_cpp_runner
{
inline namespace v1
{
namespace fs = boost::filesystem;
struct CucumberRunnerParams
{
	fs::path cucumber_exe{"cucumber"};
	std::string cucumber_options{};
	fs::path feature_path{"."};
	bool verbose{false};
};

CUCUMBER_CPP_RUNNER_EXPORT int execute_cucumber_tests(const CucumberRunnerParams & params = {});
}  // namespace v1
}  // namespace cucumber_cpp_runner
