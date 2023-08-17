/**
 * Taken and modified from CucumberCpp main.
 */
#include <cucumber_cpp_runner/cucumber_cpp_runner.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <boost/scope_exit.hpp>
#include <cucumber-cpp/internal/CukeEngineImpl.hpp>
#include <cucumber-cpp/internal/connectors/wire/WireProtocol.hpp>
#include <cucumber-cpp/internal/connectors/wire/WireServer.hpp>
#include <fmt/format.h>
#include <yaml-cpp/yaml.h>

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
 */
struct WireServer
{
	std::string host;
	int port;
	std::string unix_path;
	bool verbose;

	cucumber::internal::CukeEngineImpl cuke_engine{};
	cucumber::internal::JsonSpiritWireMessageCodec wire_codec{};
	cucumber::internal::WireProtocolHandler protocol_handler{wire_codec, cuke_engine};

	std::unique_ptr<cucumber::internal::SocketServer> socket_server =
		[&]() -> std::unique_ptr<cucumber::internal::SocketServer>
	{
		std::unique_ptr<cucumber::internal::SocketServer> server;
		if (!unix_path.empty())
		{
			auto unix_server = std::make_unique<UnixSocketServer>(&protocol_handler);
			unix_server->listen(unix_path);
			if (verbose)
				std::clog << "Listening on socket " << unix_server->listenEndpoint() << '\n';
			server = std::move(unix_server);
		}
		else
		{
			auto tcp_server = std::make_unique<TCPSocketServer>(&protocol_handler);
			boost::asio::io_service service{};
			// Use resolver, rather than ip::from_string, in case "localhost" is given.
			tcp_server->listen(boost::asio::ip::tcp::resolver{service}
								   // NOLINT(*-default-arguments-calls)
								   .resolve(host, std::to_string(port))
								   .begin()
								   ->endpoint());
			if (verbose)
				std::clog << "Listening on " << tcp_server->listenEndpoint() << "\n";
			server = std::move(tcp_server);
		}
		return server;
	}();
};

/**
 * Synchronously open socket for connections, then asynchronously process them.
 *
 * This means when we run cucumber, there will be a server to connect to, even if the
 * connections are not yet being serviced, resolving the race condition.
 */
struct WireServerThread
{
	WireServerThread(std::string host, int const port, std::string unix_path, bool const verbose)
		: server_{new WireServer{std::move(host), port, std::move(unix_path), verbose}},
		  request_{std::async(std::launch::async, [this] { server_->socket_server->acceptOnce(); })}
	{
	}
	WireServerThread(WireServerThread &&) = default;
	WireServerThread & operator=(WireServerThread &&) = delete;
	WireServerThread(WireServerThread const &) = delete;
	WireServerThread & operator=(WireServerThread const &) = delete;
	~WireServerThread()
	{
		if (request_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
			try
			{
				dynamic_cast<SocketServerStopInterface &>(*server_->socket_server).stop();
			}
			catch (std::exception const & exc)
			{
				fmt::print(stderr, "Failed to stop wire server: {}", exc.what());
			}
	}

private:
	std::unique_ptr<WireServer> server_;
	std::future<void> request_;
};

}  // namespace

boost::filesystem::path cucumber_exe_path()
{
	boost::filesystem::path path = boost::process::search_path("cucumber");
	//	if (path.empty())
	//		throw std::runtime_error{"'cucumber' executable not found"};
	return path;
}

/**
 * Launch a subprocess to execute `cucumber` command line.
 */
int run_cucumber(
	std::string const & feature_dir,
	std::string const & cucumber_exe,
	std::string const & cucumber_options,
	bool const verbose)
{
	namespace bp = boost::process;

	bp::ipstream cucumber_stdout;

	std::string const cucumber_cmd =
		fmt::format("{} {} {}", cucumber_exe, cucumber_options, feature_dir);

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

int main(int argc, char ** argv)
{
	// Ensure ctest, etc, output doesn't get interleaved.
	boost::scope_exit::aux::guard const flusher{[] { std::cout.flush(); }};

	using boost::program_options::value;
	boost::program_options::options_description cmd_options_desc("Allowed options");

	cmd_options_desc.add_options()("help,h", "help for cucumber-cpp")(
		"verbose,v", "verbose output")(
		"config,c",
		value<std::string>()->default_value("step_definitions/cucumber.wire"),
		"location of .wire config file")(
		"features,f",
		value<std::string>()->default_value("."),
		"location of feature file or directory")(
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

	if (!boost::filesystem::exists(cmd_options["config"].as<std::string>()))
	{
		fmt::print(
			stderr, "Wire config not found at '{}'", cmd_options["config"].as<std::string>());
		return kExitFailure;
	}

	std::string listen_host;
	int port{};
	std::string unix_path;

	std::string yaml;
	boost::filesystem::load_string_file(cmd_options["config"].as<std::string>(), yaml);
	auto config = YAML::Load(yaml);

	try
	{
		if (config["unix"].IsDefined())
		{
			unix_path = config["unix"].as<std::string>();
		}
		else if (config["host"].IsDefined())
		{
			listen_host = config["host"].as<std::string>();
			port = config["port"].as<int>();
		}

#ifndef BOOST_ASIO_HAS_LOCAL_SOCKETS
		if (!unixPath.empty())
		{
			fmt::print(stderr, "Unix paths are unsupported on this system: '{}'", unixPath);
			return exit_failure;
		}
#endif

		bool const verbose = cmd_options.count("verbose") != 0U;

		boost::filesystem::path const cucumber_exe = cucumber_exe_path();

		WireServerThread const wire_server{
			std::move(listen_host), port, std::move(unix_path), verbose};

		int const return_code = run_cucumber(
			cmd_options["features"].as<std::string>(),
			cucumber_exe.string(),
			cmd_options["options"].as<std::string>(),
			verbose);

		return return_code;
	}
	catch (std::exception & e)
	{
		fmt::print(stderr, "{}\n", e.what());
		return kExitFailure;
	}
}