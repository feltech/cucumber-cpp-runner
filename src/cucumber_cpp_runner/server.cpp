// Copyright 2023 David Feltell
// SPDX-License-Identifier: MIT
/**
 * Taken and modified from CucumberCpp main.
 */
#include "./server.hpp"

#include <iostream>

#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/string_file.hpp>
#include <cucumber-cpp/internal/CukeEngineImpl.hpp>
#include <cucumber-cpp/internal/connectors/wire/WireProtocol.hpp>
#include <cucumber-cpp/internal/connectors/wire/WireServer.hpp>
#include <fmt/format.h>
#include <yaml-cpp/yaml.h>

namespace
{
struct SocketServerStopInterface
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
}  // namespace

namespace cucumber_cpp_runner
{
inline namespace v1
{

struct WireServerImpl
{
	WireServerImpl(std::string host, int port, std::string unix_path, bool verbose)
		: host_(std::move(host)), port_(port), unix_path_(std::move(unix_path)), verbose_(verbose)
	{
	}
	~WireServerImpl()
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

	WireServerImpl() = delete;
	WireServerImpl(WireServerImpl &&) = delete;
	WireServerImpl & operator=(WireServerImpl &&) = delete;
	WireServerImpl(WireServerImpl const &) = delete;
	WireServerImpl & operator=(WireServerImpl const &) = delete;

private:
	std::string host_;
	int port_;
	std::string unix_path_;
	bool verbose_;

	cucumber::internal::CukeEngineImpl cuke_engine_{};
	cucumber::internal::JsonSpiritWireMessageCodec wire_codec_{};
	cucumber::internal::WireProtocolHandler protocol_handler_{wire_codec_, cuke_engine_};

	std::unique_ptr<cucumber::internal::SocketServer> socket_server_ = [&]
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

WireServer::WireServer(std::string host, int port, std::string unix_path, bool verbose)
	: impl_{std::make_unique<WireServerImpl>(std::move(host), port, std::move(unix_path), verbose)}
{
}

WireServer::~WireServer() = default;

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
}  // namespace v1
}  // namespace cucumber_cpp_runner