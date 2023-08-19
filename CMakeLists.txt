cmake_minimum_required(VERSION 3.21)

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
	cucumber_cpp_runner
	VERSION 1.0.0
	DESCRIPTION ""
	HOMEPAGE_URL "https://github.com/feltech/cucumber_cpp_runner"
	LANGUAGES CXX C)

include(cmake/PreventInSourceBuilds.cmake)
include(ProjectOptions.cmake)

cucumber_cpp_runner_setup_options()
cucumber_cpp_runner_global_options()
include(Dependencies.cmake)
cucumber_cpp_runner_setup_dependencies()
cucumber_cpp_runner_local_options()

target_compile_features(cucumber_cpp_runner_options INTERFACE cxx_std_${CMAKE_CXX_STANDARD})
add_library(cucumber_cpp_runner::cucumber_cpp_runner_options ALIAS cucumber_cpp_runner_options)
add_library(cucumber_cpp_runner::cucumber_cpp_runner_warnings ALIAS cucumber_cpp_runner_warnings)

#add_library(cucumber_cpp_runner::cucumber_cpp_runner_options INTERFACE IMPORTED)
#add_library(cucumber_cpp_runner::cucumber_cpp_runner_warnings INTERFACE IMPORTED)

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
endif ()

# If MSVC is being used, and ASAN is enabled, we need to set the debugger environment
# so that it behaves well with MSVC's debugger, and we can run the target from visual studio
if (MSVC)
	get_all_installable_targets(all_targets)
	message("all_targets=${all_targets}")
	set_target_properties(${all_targets} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=$(VC_ExecutablePath_x64);%PATH%")
endif ()

# set the startup project for the "play" button in MSVC
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT intro)

if (CMAKE_SKIP_INSTALL_RULES)
	return()
endif ()

include(cmake/PackageProject.cmake)

# Add other targets that you want installed here, by default we just package the one executable
# we know we want to ship
cucumber_cpp_runner_package_project(
	TARGETS
	cucumber_cpp_runner
	cucumber_cpp_runner_nomain
	cucumber_cpp_runner_options
	cucumber_cpp_runner_warnings
)

# GIT_SHORT_SHA is provided externally, e.g. by CI build.
# If not provided, assume a developer build.
if (NOT DEFINED GIT_SHORT_SHA)
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
	"${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${GIT_SHORT_SHA}-${CMAKE_SYSTEM_NAME}-${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}-${_library_type}"
)
message(STATUS "CPack package file name: '${CPACK_PACKAGE_FILE_NAME}'")

include(CPack)
