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
		"NAME;GIT_TAG;GITHUB_REPOSITORY"
		"FIND_PACKAGE_OPTIONS;CMAKE_OPTIONS"
	)

	list(APPEND CMAKE_PREFIX_PATH ${cucumber_cpp_runner_DEPENDENCY_INSTALL_CACHE_DIR})

	find_package(
		${args_NAME} CONFIG
		PATHS ${cucumber_cpp_runner_DEPENDENCY_INSTALL_CACHE_DIR}
		${args_FIND_PACKAGE_OPTIONS}
	)
	if (NOT DEFINED ${args_NAME}_CONFIG)
		message(STATUS "${args_NAME} not found, downloading...")

		CPMAddPackage(
			NAME ${args_NAME}
			SYSTEM YES
			EXCLUDE_FROM_ALL YES
			DOWNLOAD_ONLY YES
			GIT_TAG ${args_GIT_TAG}
			GITHUB_REPOSITORY ${args_GITHUB_REPOSITORY}
		)

		set(_build_type Release)

		if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
			# Frustratingly, need to keep a list of warnings to disable, for annoying projects
			# that force `-Werror` even for consumers (e.g. Cucumber-Cpp)
			# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105329 (affects Cucumber-Cpp)
			list(APPEND args_CMAKE_OPTIONS "-DCMAKE_CXX_FLAGS=-Wno-restrict -Wno-unknown-warning-option")
		endif ()

		if (BUILD_SHARED_LIBS)
			# In case we're linking a static library into a shared library.
			list(APPEND args_CMAKE_OPTIONS -DCMAKE_POSITION_INDEPENDENT_CODE=ON)
		elseif(DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
			list(APPEND args_CMAKE_OPTIONS -DCMAKE_POSITION_INDEPENDENT_CODE=${CMAKE_POSITION_INDEPENDENT_CODE})
		endif ()

		if (CMAKE_PREFIX_PATH)
			string(REPLACE ";" "$<SEMICOLON>" _prefix_path "${CMAKE_PREFIX_PATH}")
			list(APPEND args_CMAKE_OPTIONS "-DCMAKE_PREFIX_PATH=${_prefix_path}")
		endif ()

		if (CMAKE_TOOLCHAIN_FILE)
			# Pass along any toolchain file providing e.g. 3rd party libs via Conan.
			list(APPEND args_CMAKE_OPTIONS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
		endif ()

		list(APPEND args_CMAKE_OPTIONS
			-S "${${args_NAME}_SOURCE_DIR}"
			-B "${${args_NAME}_BINARY_DIR}"
			-G "${CMAKE_GENERATOR}"
			-DCMAKE_BUILD_TYPE=${_build_type}
			--compile-no-warning-as-error
			"-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
			# For MSVC and Conan v2
			-DCMAKE_POLICY_DEFAULT_CMP0091=NEW
			# Allow `PackageName_ROOT` hint for find_package calls.
			-DCMAKE_POLICY_DEFAULT_CMP0074=NEW
			#			-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
			-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
			#			-DCMAKE_CXX_EXTENSIONS=${CMAKE_CXX_EXTENSIONS}
			#			-DCMAKE_CXX_STANDARD_REQUIRED=${CMAKE_CXX_STANDARD_REQUIRED}
			# TODO(DF): <$IF:$<BOOL:CMAKE_FIND_PACKAGE_PREFER_CONFIG ?
			-DCMAKE_FIND_PACKAGE_PREFER_CONFIG=TRUE
		)

		message(TRACE "${args_NAME} options: ${args_CMAKE_OPTIONS}")

		execute_process(
			COMMAND ${CMAKE_COMMAND}
			${args_CMAKE_OPTIONS}
			COMMAND_ERROR_IS_FATAL ANY
		)
		execute_process(
			COMMAND ${CMAKE_COMMAND}
			--build ${${args_NAME}_BINARY_DIR} --config ${_build_type} --parallel
			COMMAND_ERROR_IS_FATAL ANY
		)
		execute_process(
			COMMAND ${CMAKE_COMMAND}
			--install ${${args_NAME}_BINARY_DIR}
			--prefix ${cucumber_cpp_runner_DEPENDENCY_INSTALL_CACHE_DIR}
			--config ${_build_type}
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

#	if (NOT TARGET GTest::gtest)
#		cucumber_cpp_runner_cpm_install_package(
#			NAME GTest
#			GITHUB_REPOSITORY google/googletest
#			GIT_TAG v1.14.0
#		)
#	endif ()
	find_package(CucumberCpp QUIET)
	if (NOT TARGET CucumberCpp::cucumber-cpp-nomain)
		set(
			_cucumber_cpp_cmake_options
			# TODO(DF): Can't build as a separate shared lib because:
			# > undefined reference to `typeinfo for cucumber::internal::CukeEngine'
			# But Cucumber-Cpp is a public dependency, so if we statically link it into a shared
			# library, then we need to re-export all its symbols, which is not trivial.
			-DBUILD_SHARED_LIBS=FALSE
			-DCUKE_ENABLE_GTEST=OFF
			# It builds the driver regardless, if the boost unit_test_framework target exists, and
			# then results in a build failure on Windows if disabled.
			-DCUKE_ENABLE_BOOST_TEST=ON
			-DCUKE_ENABLE_QT=OFF
			-DCUKE_TESTS_E2E=OFF
			-DCUKE_TESTS_UNIT=OFF
		)

		# Pass along any external override to Boost static vs. shared.
		if (DEFINED Boost_USE_STATIC_LIBS)
			set(_cucumber_cpp_cmake_options
				${_cucumber_cpp_cmake_options} -DCUKE_USE_STATIC_BOOST=${Boost_USE_STATIC_LIBS})
		endif ()

		cucumber_cpp_runner_cpm_install_package(
			NAME CucumberCpp
			GITHUB_REPOSITORY cucumber/cucumber-cpp
			GIT_TAG main
			FIND_PACKAGE_OPTIONS
			PATH_SUFFIXES lib/cmake
			CMAKE_OPTIONS
			${_cucumber_cpp_cmake_options}
		)
	endif ()

	# Disallow shared library builds of Cucumber-Cpp
	get_target_property(_cucumber_cpp_target_type CucumberCpp::cucumber-cpp-nomain TYPE)
	if (NOT _cucumber_cpp_target_type STREQUAL "STATIC_LIBRARY")
		message(FATAL_ERROR "Cucumber-Cpp must be provided as a static library")
	endif ()

	find_package(fmt QUIET)
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

	find_package(yaml-cpp QUIET)
	if (NOT TARGET yaml-cpp::yaml-cpp)
		cucumber_cpp_runner_cpm_install_package(
			NAME yaml-cpp
			GITHUB_REPOSITORY jbeder/yaml-cpp
			GIT_TAG 0.8.0
		)
	endif ()

endfunction()
