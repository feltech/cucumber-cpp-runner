/**
 * Taken and modified from CucumberCpp main.
 */
#include <cucumber_cpp_runner/cucumber_cpp_runner.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <utility>

#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <boost/scope_exit.hpp>
#include <cucumber-cpp/internal/CukeEngineImpl.hpp>
#include <cucumber-cpp/internal/connectors/wire/WireProtocol.hpp>
#include <cucumber-cpp/internal/connectors/wire/WireServer.hpp>
#include <fmt/format.h>
#include <yaml-cpp/yaml.h>

namespace fs = boost::filesystem;

namespace
{
// Silence clang-tidy readability-magic-numbers, which doesn't like macros.
constexpr int kExitSuccess = EXIT_SUCCESS;
constexpr int kExitFailure = EXIT_FAILURE;
constexpr unsigned kCmdHelpTextWidth = 80;
constexpr unsigned kCucumberTimeoutMs = 10000;

struct SocketServerStopInterface  // NOLINT(*-special-member-functions)
{
	virtual void stop() = 0;
	virtual ~SocketServerStopInterface() = default;
};

struct TCPSocketServer : cucumber::internal::TCPSocketServer, SocketServerStopInterface
{
	using cucumber::internal::TCPSocketServer::TCPSocketServer;
	void stop() override
	{
		boost::asio::io_service service;
		boost::asio::ip::tcp::socket socket{service};
		socket.connect(listenEndpoint());
		socket.send(boost::asio::buffer("some junk to make it fail"));
	}
};

struct UnixSocketServer : cucumber::internal::UnixSocketServer, SocketServerStopInterface
{
	using cucumber::internal::UnixSocketServer::UnixSocketServer;
	void stop() override
	{
		boost::asio::io_service service;
		boost::asio::local::stream_protocol::socket socket{service};
		socket.connect(listenEndpoint());
		socket.send(boost::asio::buffer("some junk to make it fail"));
	}
};

/**
 * Encapsulate the construction of a Cucumber Wire Protocol server via CucumberCpp.
 *
 * Synchronously open socket for connections, then asynchronously process them.
 *
 * This means when we run cucumber, there will be a server to connect to, even if the
 * connections are not yet being serviced, resolving the race condition.
 */
struct WireServer
{
	WireServer(std::string host, int port, std::string unix_path, bool verbose)
		: host_(std::move(host)), port_(port), unix_path_(std::move(unix_path)), verbose_(verbose)
	{
	}
	~WireServer()
	{
		if (request_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
			try
			{
				dynamic_cast<SocketServerStopInterface &>(*socket_server_).stop();
			}
			catch (std::exception const & exc)
			{
				fmt::print(stderr, "Failed to stop wire server: {}", exc.what());
			}
	}

	WireServer() = delete;
	WireServer(WireServer &&) = delete;
	WireServer & operator=(WireServer &&) = delete;
	WireServer(WireServer const &) = delete;
	WireServer & operator=(WireServer const &) = delete;

private:
	std::string host_;
	int port_;
	std::string unix_path_;
	bool verbose_;

	cucumber::internal::CukeEngineImpl cuke_engine_{};
	cucumber::internal::JsonSpiritWireMessageCodec wire_codec_{};
	cucumber::internal::WireProtocolHandler protocol_handler_{wire_codec_, cuke_engine_};

	std::unique_ptr<cucumber::internal::SocketServer> socket_server_ =
		[&]() -> std::unique_ptr<cucumber::internal::SocketServer>
	{
		std::unique_ptr<cucumber::internal::SocketServer> server;
		if (!unix_path_.empty())
		{
			auto unix_server = std::make_unique<UnixSocketServer>(&protocol_handler_);
			unix_server->listen(unix_path_);
			if (verbose_)
				std::clog << "Listening on socket " << unix_server->listenEndpoint() << '\n';
			server = std::move(unix_server);
		}
		else
		{
			auto tcp_server = std::make_unique<TCPSocketServer>(&protocol_handler_);
			boost::asio::io_service service{};
			// Use resolver, rather than ip::from_string, in case "localhost" is given.
			tcp_server->listen(boost::asio::ip::tcp::resolver{service}
								   // NOLINT(*-default-arguments-calls)
								   .resolve(host_, std::to_string(port_))
								   .begin()
								   ->endpoint());
			if (verbose_)
				std::clog << "Listening on " << tcp_server->listenEndpoint() << "\n";
			server = std::move(tcp_server);
		}
		return server;
	}();

	std::future<void> request_{
		std::async(std::launch::async, [this] { socket_server_->acceptOnce(); })};
};

}  // namespace

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
	fs::path path = boost::process::search_path(cucumber_exe);
	if (path.empty())
		throw std::runtime_error{"'cucumber' executable not found"};
	return path;
}

fs::path find_wire_config(fs::path const & feature_path)
{
	for (fs::directory_entry const & entry : fs::recursive_directory_iterator(feature_path))
	{
		if (fs::is_directory(entry))
			continue;

		fs::path file_path;
		if (fs::is_symlink(entry))
			file_path = fs::read_symlink(entry);
		else
			file_path = entry.path();

		if (!fs::is_regular_file(file_path))
			continue;

		if (file_path.extension() != ".wire")
			continue;

		return file_path;
	}
	throw std::invalid_argument{
		fmt::format(".wire file not found in directory tree {}", feature_path.string())};
}

std::tuple<std::string, int, std::string> parse_wire_config(fs::path const & wire_config_path)
{
	std::string host;
	int port{};
	std::string unix_path;

	std::string yaml;
	fs::load_string_file(wire_config_path, yaml);
	auto config = YAML::Load(yaml);

	if (config["unix"].IsDefined())
		unix_path = config["unix"].as<std::string>();

	if (config["host"].IsDefined())
	{
		if (!unix_path.empty())
			throw std::invalid_argument{fmt::format(
				"Both unix path '{}' and TCP host {}:{} defined. Only one is supported.",
				unix_path,
				host,
				port)};
		host = config["host"].as<std::string>();
		port = config["port"].as<int>();
	}

#ifndef BOOST_ASIO_HAS_LOCAL_SOCKETS
	if (!unix_path.empty())
		throw std::invalid_argument{
			fmt::print(stderr, "Unix paths are unsupported on this system: '{}'", unix_path)};
#endif

	return {std::move(host), port, std::move(unix_path)};
}

struct CucumberRunnerParams
{
	fs::path cucumber_exe{"cucumber"};
	std::string cucumber_options{};
	fs::path feature_path{"."};
	bool verbose{false};
};

int execute_cucumber_tests(const CucumberRunnerParams & params = {})
{
	// Locate now so we can error out early if not found.
	fs::path const cucumber_exe_path = find_cucumber_exe(params.feature_path);

	auto [host, port, unix_path] = parse_wire_config(find_wire_config(params.feature_path));

	WireServer const wire_server{std::move(host), port, std::move(unix_path), params.verbose};

	return run_cucumber_exe(
		params.feature_path, cucumber_exe_path.string(), params.cucumber_options, params.verbose);
}

int main(int argc, char ** argv)
{
	// Ensure ctest, etc, output doesn't get interleaved.
	boost::scope_exit::aux::guard const flusher{[] { std::cout.flush(); }};

	using boost::program_options::value;
	boost::program_options::options_description cmd_options_desc("Allowed options");

	cmd_options_desc.add_options()("help,h", "help for cucumber-cpp")(
		"verbose,v", "verbose output")(
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
		cmd_options_desc.print(std::cerr, kCmdHelpTextWidth);
		return kExitSuccess;
	}

	try
	{
		return execute_cucumber_tests(
			{cmd_options["cucumber"].as<std::string>(),
			 cmd_options["options"].as<std::string>(),
			 cmd_options["features"].as<std::string>(),
			 cmd_options["verbose"].as<bool>()});
	}
	catch (std::exception & e)
	{
		fmt::print(stderr, "{}\n", e.what());
		return kExitFailure;
	}
}