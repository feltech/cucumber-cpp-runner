/**
 * Taken and modified from CucumberCpp main.
 */
#include <cucumber-cpp-runner/cucumber_cpp_runner.hpp>

#include <boost/filesystem.hpp>

#include "./client.hpp"
#include "./server.hpp"

namespace cucumber_cpp_runner
{
inline namespace v1
{
namespace fs = boost::filesystem;

int execute_cucumber_tests(CucumberRunnerParams const & params)
{
	// Locate now so we can error out early if not found.
	fs::path const cucumber_exe_path = find_cucumber_exe(params.cucumber_exe);

	auto [host, port, unix_path] = parse_wire_config(find_wire_config(params.feature_path));

	WireServer const server{std::move(host), port, std::move(unix_path), params.verbose};

	return run_cucumber_exe(
		params.feature_path, cucumber_exe_path.string(), params.cucumber_options, params.verbose);
}
}  // namespace v1
}  // namespace cucumber_cpp_runner