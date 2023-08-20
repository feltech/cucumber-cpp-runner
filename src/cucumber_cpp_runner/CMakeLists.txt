include(GenerateExportHeader)

add_library(cucumber_cpp_runner
	client.cpp
	server.cpp
	runner.cpp
	main.cpp
	client.hpp
	server.hpp
)

# Version without main function.
add_library(cucumber_cpp_runner_nomain
	client.cpp
	server.cpp
	runner.cpp
)

foreach (target_name cucumber_cpp_runner cucumber_cpp_runner_nomain)
	add_library(cucumber_cpp_runner::${target_name} ALIAS ${target_name})

	target_link_libraries(
		${target_name}
		PUBLIC
		CucumberCpp::cucumber-cpp-nomain
		Boost::filesystem
		PRIVATE
		Boost::program_options
		cucumber_cpp_runner_options
		cucumber_cpp_runner_warnings
		yaml-cpp::yaml-cpp
		fmt::fmt
	)

	target_include_directories(
		${target_name} ${WARNING_GUARD}
		PUBLIC
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
		$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>)

	set_target_properties(
		${target_name}
		PROPERTIES VERSION ${PROJECT_VERSION}
		CXX_VISIBILITY_PRESET hidden
		VISIBILITY_INLINES_HIDDEN YES
	)

	if (NOT BUILD_SHARED_LIBS)
		target_compile_definitions(${target_name} PUBLIC CUCUMBER_CPP_RUNNER_STATIC_DEFINE)
	endif ()

endforeach ()

generate_export_header(
	cucumber_cpp_runner
	EXPORT_FILE_NAME
	${PROJECT_BINARY_DIR}/include/cucumber_cpp_runner/cucumber_cpp_runner_export.hpp
)
