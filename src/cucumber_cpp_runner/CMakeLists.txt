include(GenerateExportHeader)

add_library(cucumber_cpp_runner cucumber_cpp_runner.cpp)
add_library(cucumber_cpp_runner::cucumber_cpp_runner ALIAS cucumber_cpp_runner)

target_link_libraries(
	cucumber_cpp_runner
	PUBLIC
	CucumberCpp::cucumber-cpp
	PRIVATE
	cucumber_cpp_runner_options
	cucumber_cpp_runner_warnings
	yaml-cpp::yaml-cpp
	Boost::headers
	fmt::fmt
)

target_include_directories(cucumber_cpp_runner ${WARNING_GUARD} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
	$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>)

set_target_properties(
	cucumber_cpp_runner
	PROPERTIES VERSION ${PROJECT_VERSION}
	CXX_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN YES
)

generate_export_header(
	cucumber_cpp_runner
	EXPORT_FILE_NAME
	${PROJECT_BINARY_DIR}/include/cucumber_cpp_runner/cucumber_cpp_runner_export.hpp
)

if (NOT BUILD_SHARED_LIBS)
	target_compile_definitions(cucumber_cpp_runner PUBLIC CUCUMBER_CPP_RUNNER_STATIC_DEFINE)
endif ()
