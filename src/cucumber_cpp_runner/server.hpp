// Copyright 2023 David Feltell
// SPDX-License-Identifier: MIT
#pragma once

#include <memory>

#include <boost/filesystem/path.hpp>

namespace cucumber_cpp_runner
{
inline namespace v1
{
namespace fs = boost::filesystem;

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
	WireServer(std::string host, int port, std::string unix_path, bool verbose);
	WireServer(WireServer &&) = default;
	WireServer & operator=(WireServer &&) = default;
	WireServer(WireServer const &) = delete;
	WireServer & operator=(WireServer const &) = delete;
	~WireServer();

private:
	std::unique_ptr<struct WireServerImpl> impl_;
};

std::tuple<std::string, int, std::string> parse_wire_config(fs::path const & wire_config_path);

fs::path find_wire_config(fs::path const & feature_path);
}  // namespace v1
}  // namespace cucumber_cpp_runner