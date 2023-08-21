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
	src/features/step_definitions/test.cpp
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
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)
add_test(
	NAME cucumber_cpp_runner.default.content
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test>
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_tests_properties(cucumber_cpp_runner.default.content PROPERTIES
	PASS_REGULAR_EXPRESSION "2 scenarios \\(2 passed\\)")

# Help text

add_test(
	NAME cucumber_cpp_runner.opt.help
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test> -h
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_tests_properties(cucumber_cpp_runner.opt.help PROPERTIES
	PASS_REGULAR_EXPRESSION "Allowed options")

# Verbose mode

add_test(
	NAME cucumber_cpp_runner.opt.verbose
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test> -v
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_tests_properties(cucumber_cpp_runner.opt.verbose PROPERTIES
	PASS_REGULAR_EXPRESSION "Listening on")

# Specifying feature directory from outside of feature tree.

add_test(
	NAME cucumber_cpp_runner.opt.features.dir
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test> -v -o "--strict"
	-f ${CMAKE_CURRENT_SOURCE_DIR}/src
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Specifying a particular feature file from outside of feature tree.

add_test(
	NAME cucumber_cpp_runner.opt.features.file
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test> -v -o "--strict"
	-f ${CMAKE_CURRENT_SOURCE_DIR}/src/features/test1.feature
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_tests_properties(cucumber_cpp_runner.opt.features.file PROPERTIES
	FAIL_REGULAR_EXPRESSION "this second feature"
)

# Specifying invalid feature directory

add_test(
	NAME cucumber_cpp_runner.opt.features.bad
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test> -v -o "-v --strict"
	-f ${CMAKE_CURRENT_BINARY_DIR}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_tests_properties(cucumber_cpp_runner.opt.features.bad PROPERTIES
	PASS_REGULAR_EXPRESSION "wire file not found in directory tree '${CMAKE_CURRENT_BINARY_DIR}'")

# Passing additional options to the `cucumber` command line.

add_test(
	NAME cucumber_cpp_runner.opt.options
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test> -v -o "--dry-run"
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_tests_properties(cucumber_cpp_runner.opt.options PROPERTIES
	PASS_REGULAR_EXPRESSION "2 scenarios \\(2 skipped\\)")

# Passing a custom `cucumber` executable.

find_program(_cucumber_exe cucumber)
add_test(
	NAME cucumber_cpp_runner.opt.cucumber
	COMMAND
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test> -c ${_cucumber_exe}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Passing an invalid custom `cucumber` executable.

add_test(
	NAME cucumber_cpp_runner.opt.cucumber.fail
	COMMAND $<TARGET_FILE:cucumber_cpp_runner.test> -c non_existent_cucumber_exe
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_tests_properties(cucumber_cpp_runner.opt.cucumber.fail PROPERTIES
	PASS_REGULAR_EXPRESSION "Cucumber executable not found using 'non_existent_cucumber_exe'")

# Create a test target that launches a custom target of the same name
function(add_test_for_custom_target target_name)
	add_test(
		NAME ${target_name}
		COMMAND
		${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config $<CONFIG> --target ${target_name}
	)
endfunction()

# Create fixtures to copy features into a temporary directory and remove when done.
#
# Names the temporary directory using the target name.
function(create_tmp_features target_name)
	add_custom_target(
		${target_name}.setup.feature
		COMMAND
		${CMAKE_COMMAND} -E make_directory
		${CMAKE_CURRENT_BINARY_DIR}/${target_name}/features
		COMMAND
		${CMAKE_COMMAND} -E copy_directory
		${CMAKE_CURRENT_SOURCE_DIR}/src/features ${CMAKE_CURRENT_BINARY_DIR}/${target_name}/features
	)
	add_test_for_custom_target(${target_name}.setup.feature)
	set_tests_properties(
		${target_name}.setup.feature
		PROPERTIES
		FIXTURES_SETUP ${target_name}.setup.feature
	)
	add_custom_target(
		${target_name}.teardown.feature
		COMMAND
		${CMAKE_COMMAND} -E remove_directory
		${CMAKE_CURRENT_BINARY_DIR}/${target_name}
	)
	add_test_for_custom_target(${target_name}.teardown.feature)
	set_tests_properties(
		${target_name}.teardown.feature
		PROPERTIES
		FIXTURES_CLEANUP ${target_name}.teardown.feature
	)
endfunction()

# Create a custom target that launches the test executable in a subdir of the binary dir.
function(add_custom_target_test target_name)
	add_custom_target(
		${target_name}
		COMMAND $<TARGET_FILE:cucumber_cpp_runner.test>
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${target_name}
	)
	add_test_for_custom_target(${target_name})
endfunction()

# Symlink the .wire file

create_tmp_features(cucumber_cpp_runner.test.symlink_wire)
add_custom_target(
	cucumber_cpp_runner.test.symlink_wire.setup.wire
	COMMAND
	${CMAKE_COMMAND} -E create_symlink
	${CMAKE_CURRENT_SOURCE_DIR}/src/features/step_definitions/cucumber.wire
	${CMAKE_CURRENT_BINARY_DIR}/cucumber_cpp_runner.test.symlink_wire/features/step_definitions/cucumber.wire
)
add_test_for_custom_target(cucumber_cpp_runner.test.symlink_wire.setup.wire)
set_tests_properties(
	cucumber_cpp_runner.test.symlink_wire.setup.wire
	PROPERTIES
	FIXTURES_SETUP cucumber_cpp_runner.test.symlink_wire.setup.wire
	FIXTURES_REQUIRED cucumber_cpp_runner.test.symlink_wire.setup.feature
)
add_custom_target_test(cucumber_cpp_runner.test.symlink_wire)
list(APPEND _symlink_wire.fixtures
	cucumber_cpp_runner.test.symlink_wire.setup.wire
	cucumber_cpp_runner.test.symlink_wire.setup.feature
	cucumber_cpp_runner.test.symlink_wire.teardown.feature)
set_tests_properties(
	cucumber_cpp_runner.test.symlink_wire
	PROPERTIES
	FIXTURES_REQUIRED "${_symlink_wire.fixtures}"
)

# Invalid .wire specifying both unix path and tcp host

create_tmp_features(cucumber_cpp_runner.test.bad_wire_tcp_and_unix)
add_custom_target(
	cucumber_cpp_runner.test.bad_wire_tcp_and_unix.setup.wire
	COMMAND
	${CMAKE_COMMAND} -E copy
	${CMAKE_CURRENT_SOURCE_DIR}/resources/bad_wire_tcp_and_unix.wire
	${CMAKE_CURRENT_BINARY_DIR}/cucumber_cpp_runner.test.bad_wire_tcp_and_unix/features/step_definitions/cucumber.wire
)
add_test_for_custom_target(cucumber_cpp_runner.test.bad_wire_tcp_and_unix.setup.wire)
set_tests_properties(
	cucumber_cpp_runner.test.bad_wire_tcp_and_unix.setup.wire
	PROPERTIES
	FIXTURES_SETUP cucumber_cpp_runner.test.bad_wire_tcp_and_unix.setup.wire
	FIXTURES_REQUIRED cucumber_cpp_runner.test.bad_wire_tcp_and_unix.setup.feature
)
add_custom_target_test(cucumber_cpp_runner.test.bad_wire_tcp_and_unix)
list(APPEND _bad_wire_tcp_and_unix.fixtures
	cucumber_cpp_runner.test.bad_wire_tcp_and_unix.setup.wire
	cucumber_cpp_runner.test.bad_wire_tcp_and_unix.setup.feature
	cucumber_cpp_runner.test.bad_wire_tcp_and_unix.teardown.feature)
set_tests_properties(
	cucumber_cpp_runner.test.bad_wire_tcp_and_unix
	PROPERTIES
	FIXTURES_REQUIRED "${_bad_wire_tcp_and_unix.fixtures}"
	PASS_REGULAR_EXPRESSION "Both unix path .* and TCP host .* defined. Only one is supported"
)

# Unix socket

create_tmp_features(cucumber_cpp_runner.test.unix_wire)
add_custom_target(
	cucumber_cpp_runner.test.unix_wire.setup.wire
	COMMAND
	${CMAKE_COMMAND} -E copy
	${CMAKE_CURRENT_SOURCE_DIR}/resources/unix.wire
	${CMAKE_CURRENT_BINARY_DIR}/cucumber_cpp_runner.test.unix_wire/features/step_definitions/cucumber.wire
)
add_test_for_custom_target(cucumber_cpp_runner.test.unix_wire.setup.wire)
set_tests_properties(
	cucumber_cpp_runner.test.unix_wire.setup.wire
	PROPERTIES
	FIXTURES_SETUP cucumber_cpp_runner.test.unix_wire.setup.wire
	FIXTURES_REQUIRED cucumber_cpp_runner.test.unix_wire.setup.feature
)
add_custom_target_test(cucumber_cpp_runner.test.unix_wire)
list(APPEND _unix_wire.fixtures
	cucumber_cpp_runner.test.unix_wire.setup.wire
	cucumber_cpp_runner.test.unix_wire.setup.feature
	cucumber_cpp_runner.test.unix_wire.teardown.feature)
set_tests_properties(
	cucumber_cpp_runner.test.unix_wire
	PROPERTIES
	FIXTURES_REQUIRED "${_unix_wire.fixtures}"
)
if (WIN32)
	# Unix path unsupported on Windows
	set_tests_properties(
		cucumber_cpp_runner.test.unix_wire
		PROPERTIES
		PASS_REGULAR_EXPRESSION "Unix paths are unsupported on this system"
	)
endif ()