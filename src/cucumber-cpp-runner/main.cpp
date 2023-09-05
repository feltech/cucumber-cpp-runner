#include <cucumber-cpp-runner/cucumber_cpp_runner.hpp>
#include <cucumber-cpp-runner/cucumber_cpp_runner_export.hpp>

#include <iostream>

#include <boost/program_options.hpp>
#include <boost/scope_exit.hpp>
#include <fmt/format.h>

// Silence clang-tidy readability-magic-numbers, which doesn't like macros.
constexpr int kExitSuccess = EXIT_SUCCESS;
constexpr int kExitFailure = EXIT_FAILURE;
constexpr unsigned kCmdHelpTextWidth = 40;

CUCUMBER_CPP_RUNNER_EXPORT int main(int argc, char ** argv)
{
	// Ensure ctest, etc, output doesn't get interleaved.
	boost::scope_exit::aux::guard const flusher{[] { std::cout.flush(); }};

	using boost::program_options::value;
	boost::program_options::options_description cmd_options_desc("Allowed options");

	cmd_options_desc.add_options()("help,h", "this help")("verbose,v", "verbose output")(
		"features,f", value<std::string>()->default_value("."), "location of feature file(s)")(
		"cucumber,c", value<std::string>()->default_value("cucumber"), "cucumber executable")(
		"options,o",
		value<std::string>()->default_value(""),
		"additional cucumber options (surround in quotes for multiple)");

	boost::program_options::variables_map cmd_options;
	boost::program_options::store(
		boost::program_options::parse_command_line(argc, argv, cmd_options_desc), cmd_options);
	boost::program_options::notify(cmd_options);

	if (cmd_options.count("help") != 0U)
	{
		cmd_options_desc.print(std::cout, kCmdHelpTextWidth);
		return kExitSuccess;
	}

	try
	{
		return cucumber_cpp_runner::execute_cucumber_tests(
			{cmd_options["cucumber"].as<std::string>(),
			 cmd_options["options"].as<std::string>(),
			 cmd_options["features"].as<std::string>(),
			 static_cast<bool>(cmd_options.count("verbose"))});
	}
	catch (std::exception & e)
	{
		fmt::print(stderr, "{}\n", e.what());
		return kExitFailure;
	}
	catch (...)
	{
		fmt::print(stderr, "Unknown exception type caught");
		return kExitFailure;
	}
}