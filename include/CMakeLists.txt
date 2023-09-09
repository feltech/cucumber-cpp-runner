add_library(cucumber-cpp-runner-headers INTERFACE)
include(GenerateExportHeader)
generate_export_header(
	cucumber-cpp-runner-headers
	EXPORT_FILE_NAME
	${PROJECT_BINARY_DIR}/include/cucumber-cpp-runner/cucumber_cpp_runner_export.hpp
)
target_sources(
	cucumber-cpp-runner-headers
	INTERFACE
	FILE_SET headers
	TYPE HEADERS
	BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_BINARY_DIR}/include
	FILES
	cucumber-cpp-runner/cucumber_cpp_runner.hpp
	${PROJECT_BINARY_DIR}/include/cucumber-cpp-runner/cucumber_cpp_runner_export.hpp
)
if (NOT TEST_INSTALLED_VERSION)
	add_library(cucumber-cpp-runner::cucumber-cpp-runner-headers ALIAS cucumber-cpp-runner-headers)
endif ()
