include(cmake/CPM.cmake)

# Allow dependencies with e.g. only Release CMake config files to work with Debug project builds.
set(CMAKE_MAP_IMPORTED_CONFIG_RELEASE Release;RelWithDebInfo;Debug)
set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO RelWithDebInfo;Release;Debug)
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Debug;RelWithDebInfo;Release)

if (NOT DEFINED CMAKE_FIND_PACKAGE_PREFER_CONFIG)
	set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
endif ()
message(STATUS "CMAKE_FIND_PACKAGE_PREFER_CONFIG is ${CMAKE_FIND_PACKAGE_PREFER_CONFIG}")

set(
	cucumber_cpp_runner_DEPENDENCY_INSTALL_CACHE_DIR ${PROJECT_BINARY_DIR}/_deps/dist
	CACHE PATH "Location to install any missing dependencies prior to the build process"
)

function(cucumber_cpp_runner_cpm_install_package)
	cmake_parse_arguments(
		PARSE_ARGV 0
		args
		""
		"NAME;GIT_TAG;GITHUB_REPOSITORY;FIND_PACKAGE_IS_FOUND_VAR_TO_CHECK"
		"FIND_PACKAGE_OPTIONS;CMAKE_OPTIONS"
	)

	list(APPEND CMAKE_PREFIX_PATH ${cucumber_cpp_runner_DEPENDENCY_INSTALL_CACHE_DIR})

	CPMAddPackage(
		NAME ${args_NAME}
		SYSTEM YES
		EXCLUDE_FROM_ALL YES
		DOWNLOAD_ONLY YES
		GIT_TAG ${args_GIT_TAG}
		GITHUB_REPOSITORY ${args_GITHUB_REPOSITORY}
	)

	find_package(
		${args_NAME}
		PATHS ${cucumber_cpp_runner_DEPENDENCY_INSTALL_CACHE_DIR}
		${args_FIND_PACKAGE_OPTIONS}
	)
	if (NOT DEFINED ${args_NAME}_CONFIG)
		message(STATUS "${args_NAME} not found, downloading...")

		set(build_type Release)
		if (CMAKE_BUILD_TYPE)
			set(build_type ${CMAKE_BUILD_TYPE})
		endif ()

		if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
			# Frustratingly, need to keep a list of warnings to disable, for annoying projects
			# that force `-Werror` even for consumers (e.g. Cucumber-Cpp)
			# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105329 (affects Cucumber-Cpp)
			set(_compile_flags "-DCMAKE_CXX_FLAGS=-Wno-restrict -Wno-unknown-warning-option")
		endif ()

		execute_process(
			COMMAND ${CMAKE_COMMAND}
			-S ${${args_NAME}_SOURCE_DIR}
			-B ${${args_NAME}_BINARY_DIR}
			-G "${CMAKE_GENERATOR}"
			--compile-no-warning-as-error
			-DCMAKE_BUILD_TYPE=${build_type}
			-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
			${_compile_flags}
			#			-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
			-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
			#			-DCMAKE_CXX_EXTENSIONS=${CMAKE_CXX_EXTENSIONS}
			#			-DCMAKE_CXX_STANDARD_REQUIRED=${CMAKE_CXX_STANDARD_REQUIRED}
			"-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
			-DCMAKE_FIND_PACKAGE_PREFER_CONFIG=TRUE
			# Allow `PackageName_ROOT` hint for find_package calls.
			-DCMAKE_POLICY_DEFAULT_CMP0074=NEW
			${args_CMAKE_OPTIONS}
			COMMAND_ERROR_IS_FATAL ANY
		)
		execute_process(
			COMMAND ${CMAKE_COMMAND}
			--build ${${args_NAME}_BINARY_DIR} --config ${build_type} --parallel
			COMMAND_ERROR_IS_FATAL ANY
		)
		execute_process(
			COMMAND ${CMAKE_COMMAND}
			--install ${${args_NAME}_BINARY_DIR}
			--prefix ${cucumber_cpp_runner_DEPENDENCY_INSTALL_CACHE_DIR}
			COMMAND_ERROR_IS_FATAL ANY
		)
		find_package(
			${args_NAME} CONFIG REQUIRED
			PATHS ${cucumber_cpp_runner_DEPENDENCY_INSTALL_CACHE_DIR}
			${args_FIND_PACKAGE_OPTIONS}
		)
	endif ()
endfunction()


# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
# For each dependency, see if it's
# already been provided to us by a parent project
function(cucumber_cpp_runner_setup_dependencies)

	# A bit heavyweight to bring in through CPM - so require external provision.
	find_package(Boost REQUIRED)

	if (NOT TARGET yaml-cpp::yaml-cpp)
		cucumber_cpp_runner_cpm_install_package(
			NAME GTest
			GITHUB_REPOSITORY google/googletest
			GIT_TAG v1.14.0
		)
	endif ()
	if (NOT TARGET CucumberCpp::cucumber-cpp)
		cucumber_cpp_runner_cpm_install_package(
			NAME CucumberCpp
			GITHUB_REPOSITORY cucumber/cucumber-cpp
			GIT_TAG main
			FIND_PACKAGE_OPTIONS
			PATH_SUFFIXES lib/cmake
			CMAKE_OPTIONS
			-DCUKE_ENABLE_GTEST=OFF
			-DCUKE_ENABLE_BOOST_TEST=OFF
			-DCUKE_ENABLE_QT=OFF
			-DCUKE_TESTS_E2E=OFF
			-DCUKE_TESTS_UNIT=OFF
			-DBoost_ROOT=${Boost_DIR}
			-DCMAKE_VERBOSE_MAKEFILE=ON
		)
	endif ()
	if (NOT TARGET fmt::fmt)
		cucumber_cpp_runner_cpm_install_package(
			NAME fmt
			GITHUB_REPOSITORY fmtlib/fmt
			GIT_TAG 9.1.0
			CMAKE_OPTIONS
			-DFMT_TEST=OFF
			-DFMT_DOC=OFF
		)
	endif ()
	if (NOT TARGET yaml-cpp::yaml-cpp)
		cucumber_cpp_runner_cpm_install_package(
			NAME yaml-cpp
			GITHUB_REPOSITORY jbeder/yaml-cpp
			GIT_TAG 0.8.0
		)
	endif ()

endfunction()
