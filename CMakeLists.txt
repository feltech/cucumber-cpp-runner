cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0091 NEW)

# Only set the cxx_standard if it is not set by someone else
if (NOT DEFINED CMAKE_CXX_STANDARD)
	set(CMAKE_CXX_STANDARD 17)
endif ()

# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled
if (NOT DEFINED CMAKE_CXX_EXTENSIONS)
	set(CMAKE_CXX_EXTENSIONS OFF)
endif ()

# Set the project name and language
project(
	cucumber-cpp-runner
	VERSION 1.0.0
	DESCRIPTION "Execute C++ Gherkin tests with both client and server in a single main"
	HOMEPAGE_URL "https://github.com/feltech/cucumber-cpp-runner"
	LANGUAGES CXX C)

include(cmake/PreventInSourceBuilds.cmake)
include(ProjectOptions.cmake)

if (BUILD_SHARED_LIBS)
	# The dependency tree between this project, Cucumber-Cpp and Boost needs careful wrangling to
	# avoid undefined reference errors and ODR violations. E.g. Cucumber-Cpp and Boost could also
	# be built/linked as either shared or static libraries, making the build matrix hard to reason
	# about. Further, Cucumber-Cpp does not export all symbols required by this project (i.e
	# CukeEngine), so they're only accessible via static linking.
	message(FATAL_ERROR "Building as a shared library is not currently supported.")
endif ()
set(BUILD_SHARED_LIBS OFF)  # For avoidance of doubt.

cucumber_cpp_runner_setup_options()
cucumber_cpp_runner_global_options()
include(Dependencies.cmake)
cucumber_cpp_runner_setup_dependencies()
cucumber_cpp_runner_local_options()

target_compile_features(cucumber-cpp-runner-options INTERFACE cxx_std_${CMAKE_CXX_STANDARD})
if (NOT TEST_INSTALLED_VERSION)
	add_library(cucumber-cpp-runner::cucumber-cpp-runner-options ALIAS cucumber-cpp-runner-options)
	add_library(cucumber-cpp-runner::cucumber-cpp-runner-warnings ALIAS cucumber-cpp-runner-warnings)
endif ()

# Adding the src:
add_subdirectory(src)

# Don't even look at tests if we're not top level
if (NOT PROJECT_IS_TOP_LEVEL)
	return()
endif ()

if (cucumber_cpp_runner_ENABLE_TESTS)
	enable_testing()
	message(STATUS "Enabling test targets")
	add_subdirectory(test)

	# set the startup project for the "play" button in MSVC
	set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT cucumber-cpp-runner.test)
endif ()

# If MSVC is being used, and ASAN is enabled, we need to set the debugger environment
# so that it behaves well with MSVC's debugger, and we can run the target from visual studio
if (MSVC)
	get_all_installable_targets(all_targets)
	message("all_targets=${all_targets}")
	set_target_properties(${all_targets} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=$(VC_ExecutablePath_x64);%PATH%")
endif ()

if (CMAKE_SKIP_INSTALL_RULES)
	return()
endif ()

include(cmake/PackageProject.cmake)

cucumber_cpp_runner_package_project(
	TARGETS
	cucumber-cpp-runner
	cucumber-cpp-runner-nomain
	cucumber-cpp-runner-options
	cucumber-cpp-runner-warnings
)

# GIT_SHORT_SHA is provided externally, e.g. by CI build.
# If not provided, assume a developer build.
if (NOT DEFINED GIT_SHORT_SHA)
	if (NOT DEFINED GIT_SHA AND DEFINED ENV{GIT_SHA})
		set(GIT_SHA $ENV{GIT_SHA})
	endif ()

	if (DEFINED GIT_SHA)
		string(
			SUBSTRING "${GIT_SHA}"
			0
			8
			GIT_SHORT_SHA)
	else ()
		set(GIT_SHORT_SHA dev)
	endif ()
endif ()

# Experience shows that explicit package naming can help make it easier to sort
# out potential ABI related issues before they start, while helping you
# track a build to a specific GIT SHA
set(_library_type static)
if (BUILD_SHARED_LIBS)
	set(_library_type shared)
endif ()
set(CPACK_PACKAGE_FILE_NAME
	"${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${GIT_SHORT_SHA}-${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}-${_library_type}"
)
message(STATUS "CPack package file name: '${CPACK_PACKAGE_FILE_NAME}'")

include(CPack)
