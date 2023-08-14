cmake_minimum_required(VERSION 3.15...3.23)

project(CmakeConfigPackageTests LANGUAGES CXX)

if (PROJECT_IS_TOP_LEVEL OR TEST_INSTALLED_VERSION)
	enable_testing()

	find_package(cucumber_cpp_runner CONFIG REQUIRED)

	if (NOT TARGET cucumber_cpp_runner_options)
		message(FATAL_ERROR "Required config package not found!")
		return()
	endif ()
endif ()

add_executable(
	cucumber_cpp_runner.test
	features/step_definitions/test.cpp
)

target_link_libraries(
	cucumber_cpp_runner.test
	PRIVATE
	Boost::headers  # Dependency of CucumberCpp
	cucumber_cpp_runner::cucumber_cpp_runner
)

add_test(
	NAME cucumber_cpp_runner.default
	COMMAND cucumber_cpp_runner.test
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/features
)