// Copyright 2023 David Feltell
// SPDX-License-Identifier: MIT
#pragma once

#include <string>

#include <boost/filesystem/path.hpp>

namespace cucumber_cpp_runner
{
inline namespace v1
{
namespace fs = boost::filesystem;

fs::path find_cucumber_exe(fs::path const & cucumber_exe);

int run_cucumber_exe(
	fs::path const & feature_path,
	std::string const & cucumber_exe,
	std::string const & cucumber_options,
	bool verbose);
}  // namespace v1
}  // namespace cucumber_cpp_runner