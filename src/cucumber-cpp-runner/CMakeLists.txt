include(GenerateExportHeader)
add_library(cucumber-cpp-runner main.cpp)

generate_export_header(
	cucumber-cpp-runner
	EXPORT_FILE_NAME
	${PROJECT_BINARY_DIR}/include/cucumber-cpp-runner/cucumber_cpp_runner_export.hpp
)

# Version without main function.
add_library(cucumber-cpp-runner-nomain
	client.cpp
	server.cpp
	runner.cpp
)

if (NOT TEST_INSTALLED_VERSION)
	add_library(cucumber-cpp-runner::cucumber-cpp-runner ALIAS cucumber-cpp-runner)
	add_library(cucumber-cpp-runner::cucumber-cpp-runner-nomain ALIAS cucumber-cpp-runner-nomain)
endif ()

target_link_libraries(
	cucumber-cpp-runner-nomain
	PUBLIC
	CucumberCpp::cucumber-cpp-nomain
	Boost::filesystem
	PRIVATE
	cucumber-cpp-runner-options
	cucumber-cpp-runner-warnings
	yaml-cpp::yaml-cpp
	fmt::fmt
)
target_sources(
	cucumber-cpp-runner-nomain
	PUBLIC
	FILE_SET headers
	TYPE HEADERS
	BASE_DIRS ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
	FILES
	${PROJECT_SOURCE_DIR}/include/cucumber-cpp-runner/cucumber_cpp_runner.hpp
	${PROJECT_BINARY_DIR}/include/cucumber-cpp-runner/cucumber_cpp_runner_export.hpp
)

set_target_properties(
	cucumber-cpp-runner-nomain
	PROPERTIES VERSION ${PROJECT_VERSION}
	CXX_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN YES
)

if (WIN32)
	target_compile_definitions(
		cucumber-cpp-runner-nomain
		PRIVATE
		# For branching by platform within code.
		CUCUMBER_CPP_RUNNER_IS_WIN32=1
	)
endif ()

if (NOT BUILD_SHARED_LIBS)
	target_compile_definitions(cucumber-cpp-runner-nomain PUBLIC CUCUMBER_CPP_RUNNER_STATIC_DEFINE)
endif ()


target_link_libraries(
	cucumber-cpp-runner
	PUBLIC
	cucumber-cpp-runner-nomain
	PRIVATE
	cucumber-cpp-runner-options
	cucumber-cpp-runner-warnings
	Boost::program_options
	fmt::fmt
)
