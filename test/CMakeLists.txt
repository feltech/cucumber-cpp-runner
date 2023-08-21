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
	cucumber_cpp_runner::cucumber_cpp_runner
	# Enable coverage, etc.
	cucumber_cpp_runner::cucumber_cpp_runner_options
)

# Default happy path.

add_test(
	NAME cucumber_cpp_runner.default.exit_code
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test>
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
	NAME cucumber_cpp_runner.default.content
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test>
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(cucumber_cpp_runner.default.content PROPERTIES
	PASS_REGULAR_EXPRESSION "2 scenarios \\(2 passed\\)")

# Help text

add_test(
	NAME cucumber_cpp_runner.opt.help
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test> -h
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(cucumber_cpp_runner.opt.help PROPERTIES
	PASS_REGULAR_EXPRESSION "Allowed options")

# Verbose mode

add_test(
	NAME cucumber_cpp_runner.opt.verbose
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test> -v
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(cucumber_cpp_runner.opt.verbose PROPERTIES
	PASS_REGULAR_EXPRESSION "Listening on")

# Specifying feature directory from outside of feature tree.

add_test(
	NAME cucumber_cpp_runner.opt.features.dir
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test> -v -o "--strict"
	-f ${CMAKE_CURRENT_SOURCE_DIR}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Specifying a particular feature file from outside of feature tree.

add_test(
	NAME cucumber_cpp_runner.opt.features.file
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test> -v -o "--strict"
	-f ${CMAKE_CURRENT_SOURCE_DIR}/features/test1.feature
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_tests_properties(cucumber_cpp_runner.opt.features.file PROPERTIES
	FAIL_REGULAR_EXPRESSION "this second feature"
)

# Specifying invalid feature directory

add_test(
	NAME cucumber_cpp_runner.opt.features.bad
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test> -v -o "--strict"
	-f ${CMAKE_CURRENT_BINARY_DIR}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(cucumber_cpp_runner.opt.features.bad PROPERTIES
	PASS_REGULAR_EXPRESSION "wire file not found in directory tree '${CMAKE_CURRENT_BINARY_DIR}'")

# Passing additional options to the `cucumber` command line.

add_test(
	NAME cucumber_cpp_runner.opt.options
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test> -v -o "--dry-run"
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(cucumber_cpp_runner.opt.options PROPERTIES
	PASS_REGULAR_EXPRESSION "2 scenarios \\(2 skipped\\)")

# Passing a custom `cucumber` executable.

find_program(_cucumber_exe cucumber)
add_custom_target(
	cucumber_cpp_runner.test.custom_cucumber
	COMMAND
	${CMAKE_COMMAND} -E copy_if_different
	"${_cucumber_exe}" "${CMAKE_CURRENT_BINARY_DIR}/customcucumber"
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test> -c "${CMAKE_CURRENT_BINARY_DIR}/customcucumber"
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
	NAME cucumber_cpp_runner.opt.cucumber
	COMMAND
	${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target cucumber_cpp_runner.test.custom_cucumber
	--config $<CONFIG>
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Passing an invalid custom `cucumber` executable.

add_test(
	NAME cucumber_cpp_runner.opt.cucumber.fail
	COMMAND
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test> -c non_existent_cucumber_exe
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(cucumber_cpp_runner.opt.cucumber.fail PROPERTIES
	PASS_REGULAR_EXPRESSION "Cucumber executable not found using 'non_existent_cucumber_exe'")


