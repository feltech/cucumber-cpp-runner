cmake_minimum_required(VERSION 3.15...3.23)

project(CmakeConfigPackageTests LANGUAGES CXX)

if (PROJECT_IS_TOP_LEVEL OR TEST_INSTALLED_VERSION)
	enable_testing()

	find_package(cucumber-cpp-runner CONFIG REQUIRED)

	if (NOT TARGET cucumber-cpp-runner-options)
		message(FATAL_ERROR "Required config package not found!")
		return()
	endif ()
endif ()


#---------------------------------------------------------------------------------------------------
# Simple cucumber test executable/server wrapper

add_executable(
	cucumber-cpp-runner.test
	src/features/step_definitions/test.cpp
)

target_link_libraries(
	cucumber-cpp-runner.test
	PRIVATE
	cucumber-cpp-runner::cucumber-cpp-runner
	cucumber-cpp-runner::cucumber-cpp-runner-options
)


#---------------------------------------------------------------------------------------------------
# Default happy path.

add_test(
	NAME cucumber-cpp-runner.default.exit_code
	COMMAND $<TARGET_FILE:cucumber-cpp-runner.test>
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)
add_test(
	NAME cucumber-cpp-runner.default.content
	COMMAND $<TARGET_FILE:cucumber-cpp-runner.test>
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_tests_properties(cucumber-cpp-runner.default.content PROPERTIES
	PASS_REGULAR_EXPRESSION "2 scenarios \\(2 passed\\)")


#---------------------------------------------------------------------------------------------------
# Help text

add_test(
	NAME cucumber-cpp-runner.opt.help
	COMMAND $<TARGET_FILE:cucumber-cpp-runner.test> -h
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_tests_properties(cucumber-cpp-runner.opt.help PROPERTIES
	PASS_REGULAR_EXPRESSION "Allowed options")

# Verbose mode

add_test(
	NAME cucumber-cpp-runner.opt.verbose
	COMMAND $<TARGET_FILE:cucumber-cpp-runner.test> -v
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_tests_properties(cucumber-cpp-runner.opt.verbose PROPERTIES
	PASS_REGULAR_EXPRESSION "Listening on")


#---------------------------------------------------------------------------------------------------
# Specifying feature directory from outside of feature tree.

add_test(
	NAME cucumber-cpp-runner.opt.features.dir
	COMMAND $<TARGET_FILE:cucumber-cpp-runner.test> -v -o "--strict"
	-f ${CMAKE_CURRENT_SOURCE_DIR}/src
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)


#---------------------------------------------------------------------------------------------------
# Specifying a particular feature file from outside of feature tree.

add_test(
	NAME cucumber-cpp-runner.opt.features.file
	COMMAND $<TARGET_FILE:cucumber-cpp-runner.test> -v -o "--strict"
	-f ${CMAKE_CURRENT_SOURCE_DIR}/src/features/test1.feature
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_tests_properties(cucumber-cpp-runner.opt.features.file PROPERTIES
	FAIL_REGULAR_EXPRESSION "this second feature"
)


#---------------------------------------------------------------------------------------------------
# Specifying invalid feature directory

add_test(
	NAME cucumber-cpp-runner.opt.features.bad
	COMMAND $<TARGET_FILE:cucumber-cpp-runner.test> -v -o "-v --strict"
	-f ${CMAKE_CURRENT_BINARY_DIR}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_tests_properties(cucumber-cpp-runner.opt.features.bad PROPERTIES
	PASS_REGULAR_EXPRESSION "wire file not found in directory tree '${CMAKE_CURRENT_BINARY_DIR}'")


#---------------------------------------------------------------------------------------------------
# Passing additional options to the `cucumber` command line.

add_test(
	NAME cucumber-cpp-runner.opt.options
	COMMAND $<TARGET_FILE:cucumber-cpp-runner.test> -v -o "--dry-run"
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_tests_properties(cucumber-cpp-runner.opt.options PROPERTIES
	PASS_REGULAR_EXPRESSION "2 scenarios \\(2 skipped\\)")


#---------------------------------------------------------------------------------------------------
# Passing a custom `cucumber` executable.

# Win32 cucumber - we must locate the .bat file, not the script: https://gitlab.kitware.com/cmake/cmake/-/issues/16453
find_program(_cucumber_exe_path NAMES cucumber.bat cucumber REQUIRED)
cmake_path(NATIVE_PATH _cucumber_exe_path _cucumber_exe_path_native)
add_test(
	NAME cucumber-cpp-runner.opt.cucumber
	COMMAND
	COMMAND $<TARGET_FILE:cucumber-cpp-runner.test> -c ${_cucumber_exe_path_native}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)


#---------------------------------------------------------------------------------------------------
# Passing an nonexistent custom `cucumber` executable.

add_test(
	NAME cucumber-cpp-runner.opt.cucumber.nonexistent
	COMMAND $<TARGET_FILE:cucumber-cpp-runner.test> -c non_existent_cucumber_exe
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_tests_properties(cucumber-cpp-runner.opt.cucumber.nonexistent PROPERTIES
	PASS_REGULAR_EXPRESSION "Cucumber executable not found using 'non_existent_cucumber_exe'")


#---------------------------------------------------------------------------------------------------
# Invalid cucumber executable

add_test(
	NAME cucumber-cpp-runner.opt.cucumber.bad_exe
	COMMAND
	# If we have cucumber command line, then surely we have ruby
	COMMAND $<TARGET_FILE:cucumber-cpp-runner.test> -v -c ruby -o "--help"
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)


#---------------------------------------------------------------------------------------------------
# Functions for more complex tests.

# Create a simple fixture test in an isolate directory.
function(add_custom_target_test_with_tmp_fixtures target_name fixtures_list)
	create_tmp_features(${target_name})
	add_custom_target_test(${target_name} ${ARGN})
	list(APPEND ${fixtures_list}
		${target_name}.setup.feature
		${target_name}.teardown.feature)
	set_tests_properties(
		${target_name}
		PROPERTIES
		FIXTURES_REQUIRED "${${fixtures_list}}"
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
		COMMAND $<TARGET_FILE:cucumber-cpp-runner.test> ${ARGN}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${target_name}
	)
	add_dependencies(${target_name} cucumber-cpp-runner.test)
	add_test_for_custom_target(${target_name})
endfunction()

# Create a test target that launches a custom target of the same name
function(add_test_for_custom_target target_name)
	add_test(
		NAME ${target_name}
		COMMAND
		${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config $<CONFIG> --target ${target_name}
	)
endfunction()


#---------------------------------------------------------------------------------------------------
# Symlink the .wire file

add_custom_target(
	cucumber-cpp-runner.test.symlink_wire.setup.wire
	COMMAND
	${CMAKE_COMMAND} -E $<IF:$<BOOL:${WIN32}>,copy,create_symlink>
	${CMAKE_CURRENT_SOURCE_DIR}/src/features/step_definitions/cucumber.wire
	${CMAKE_CURRENT_BINARY_DIR}/cucumber-cpp-runner.test.symlink_wire/features/step_definitions/cucumber.wire
)
add_test_for_custom_target(cucumber-cpp-runner.test.symlink_wire.setup.wire)
set_tests_properties(
	cucumber-cpp-runner.test.symlink_wire.setup.wire
	PROPERTIES
	FIXTURES_SETUP cucumber-cpp-runner.test.symlink_wire.setup.wire
	FIXTURES_REQUIRED cucumber-cpp-runner.test.symlink_wire.setup.feature
)
list(APPEND _symlink_wire.fixtures
	cucumber-cpp-runner.test.symlink_wire.setup.wire)

add_custom_target_test_with_tmp_fixtures(
	cucumber-cpp-runner.test.symlink_wire _symlink_wire.fixtures)


#---------------------------------------------------------------------------------------------------
# Invalid .wire specifying both unix path and tcp host

add_custom_target(
	cucumber-cpp-runner.test.bad_wire_tcp_and_unix.setup.wire
	COMMAND
	${CMAKE_COMMAND} -E copy
	${CMAKE_CURRENT_SOURCE_DIR}/resources/bad_wire_tcp_and_unix.wire
	${CMAKE_CURRENT_BINARY_DIR}/cucumber-cpp-runner.test.bad_wire_tcp_and_unix/features/step_definitions/cucumber.wire
)
add_test_for_custom_target(cucumber-cpp-runner.test.bad_wire_tcp_and_unix.setup.wire)

list(APPEND _bad_wire_tcp_and_unix.fixtures
	cucumber-cpp-runner.test.bad_wire_tcp_and_unix.setup.wire)

add_custom_target_test_with_tmp_fixtures(
	cucumber-cpp-runner.test.bad_wire_tcp_and_unix _bad_wire_tcp_and_unix.fixtures)

set_tests_properties(
	cucumber-cpp-runner.test.bad_wire_tcp_and_unix.setup.wire
	PROPERTIES
	FIXTURES_SETUP cucumber-cpp-runner.test.bad_wire_tcp_and_unix.setup.wire
	FIXTURES_REQUIRED cucumber-cpp-runner.test.bad_wire_tcp_and_unix.setup.feature
)
set_tests_properties(
	cucumber-cpp-runner.test.bad_wire_tcp_and_unix
	PROPERTIES
	PASS_REGULAR_EXPRESSION "Both unix path .* and TCP host .* defined. Only one is supported"
)


#---------------------------------------------------------------------------------------------------
# Unix socket

add_custom_target(
	cucumber-cpp-runner.test.unix_wire.setup.wire
	COMMAND
	${CMAKE_COMMAND} -E copy
	${CMAKE_CURRENT_SOURCE_DIR}/resources/unix.wire
	${CMAKE_CURRENT_BINARY_DIR}/cucumber-cpp-runner.test.unix_wire/features/step_definitions/cucumber.wire
)
add_test_for_custom_target(cucumber-cpp-runner.test.unix_wire.setup.wire)
set_tests_properties(
	cucumber-cpp-runner.test.unix_wire.setup.wire
	PROPERTIES
	FIXTURES_SETUP cucumber-cpp-runner.test.unix_wire.setup.wire
	FIXTURES_REQUIRED cucumber-cpp-runner.test.unix_wire.setup.feature
)
list(APPEND _unix_wire.fixtures
	cucumber-cpp-runner.test.unix_wire.setup.wire)

add_custom_target_test_with_tmp_fixtures(
	cucumber-cpp-runner.test.unix_wire _unix_wire.fixtures)

if (WIN32)
	# Unix path unsupported on Windows
	set_tests_properties(
		cucumber-cpp-runner.test.unix_wire
		PROPERTIES
		PASS_REGULAR_EXPRESSION "Unix paths are unsupported on this system"
	)
endif()


#---------------------------------------------------------------------------------------------------
# Invalid cucumber exe with unix socket

add_custom_target(
	cucumber-cpp-runner.test.unix_wire_invalid_cucumber.setup.wire
	COMMAND
	${CMAKE_COMMAND} -E copy
	${CMAKE_CURRENT_SOURCE_DIR}/resources/unix.wire
	${CMAKE_CURRENT_BINARY_DIR}/cucumber-cpp-runner.test.unix_wire_invalid_cucumber/features/step_definitions/cucumber.wire
)
add_test_for_custom_target(cucumber-cpp-runner.test.unix_wire_invalid_cucumber.setup.wire)
set_tests_properties(
	cucumber-cpp-runner.test.unix_wire_invalid_cucumber.setup.wire
	PROPERTIES
	FIXTURES_SETUP cucumber-cpp-runner.test.unix_wire_invalid_cucumber.setup.wire
	FIXTURES_REQUIRED cucumber-cpp-runner.test.unix_wire_invalid_cucumber.setup.feature
)
list(APPEND _unix_wire_invalid_cucumber.fixtures
	cucumber-cpp-runner.test.unix_wire_invalid_cucumber.setup.wire)

add_custom_target_test_with_tmp_fixtures(
	cucumber-cpp-runner.test.unix_wire_invalid_cucumber
	_unix_wire_invalid_cucumber.fixtures
	-v -c ruby -o "--help"
)

if (WIN32)
	# Unix path unsupported on Windows
	set_tests_properties(
		cucumber-cpp-runner.test.unix_wire_invalid_cucumber
		PROPERTIES
		PASS_REGULAR_EXPRESSION "Unix paths are unsupported on this system"
	)
endif ()


#---------------------------------------------------------------------------------------------------
# Custom main

add_executable(
	cucumber-cpp-runner.test.nomain
	src/features/step_definitions/test.cpp
	src/custom_main.cpp
)

target_link_libraries(
	cucumber-cpp-runner.test.nomain
	PRIVATE
	cucumber-cpp-runner::cucumber-cpp-runner-nomain
	cucumber-cpp-runner::cucumber-cpp-runner-options
)

add_test(
	NAME cucumber-cpp-runner.nomain.exit_code
	COMMAND cucumber-cpp-runner.test.nomain
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)
add_test(
	NAME cucumber-cpp-runner.nomain.content
	COMMAND cucumber-cpp-runner.test.nomain
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_tests_properties(cucumber-cpp-runner.nomain.content PROPERTIES
	PASS_REGULAR_EXPRESSION "2 scenarios \\(2 passed\\)")

add_test(
	NAME cucumber-cpp-runner.nomain.error
	COMMAND cucumber-cpp-runner.test.nomain
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_tests_properties(cucumber-cpp-runner.nomain.error PROPERTIES
	PASS_REGULAR_EXPRESSION "Failed custom main: .wire file not found in directory tree '.'")