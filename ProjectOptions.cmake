include(cmake/SystemLink.cmake)
include(cmake/LibFuzzer.cmake)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)


macro(cucumber_cpp_runner_supports_sanitizers)
	if ((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)
		set(SUPPORTS_UBSAN ON)
	else ()
		set(SUPPORTS_UBSAN OFF)
	endif ()

	if ((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
		set(SUPPORTS_ASAN OFF)
	else ()
		set(SUPPORTS_ASAN ON)
	endif ()
endmacro()

macro(cucumber_cpp_runner_setup_options)
	option(cucumber_cpp_runner_ENABLE_DEVELOPER_DEFAULTS
		"Set defaults of all options for development/testing, e.g. enable tests, linters, etc" OFF)
	if (WIN32)
		# VS2019 locally: LINK : fatal error LNK1000: Internal error during IMAGE::BuildImage
		option(cucumber_cpp_runner_ENABLE_IPO "Enable IPO/LTO" OFF)
	else ()
		option(cucumber_cpp_runner_ENABLE_IPO "Enable IPO/LTO" ON)
	endif ()
	option(cucumber_cpp_runner_ENABLE_CACHE "Enable ccache" ON)
	option(cucumber_cpp_runner_ENABLE_HARDENING "Enable hardening" ON)
	option(cucumber_cpp_runner_ENABLE_COVERAGE "Enable coverage reporting" OFF)
	cmake_dependent_option(
		cucumber_cpp_runner_ENABLE_GLOBAL_HARDENING
		"Attempt to push hardening options to built dependencies"
		ON
		cucumber_cpp_runner_ENABLE_HARDENING
		OFF)

	cucumber_cpp_runner_supports_sanitizers()

	if (NOT cucumber_cpp_runner_ENABLE_DEVELOPER_DEFAULTS)
		option(cucumber_cpp_runner_ENABLE_TESTS "Enable test targets" OFF)
		option(cucumber-cpp-runner-WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
		option(cucumber_cpp_runner_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
		option(cucumber_cpp_runner_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
		option(cucumber_cpp_runner_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
		option(cucumber_cpp_runner_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
		option(cucumber_cpp_runner_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
		option(cucumber_cpp_runner_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
		option(cucumber_cpp_runner_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
		option(cucumber_cpp_runner_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
		option(cucumber_cpp_runner_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
		option(cucumber_cpp_runner_ENABLE_INCLUDE_WHAT_YOU_USE "Enable include-what-you-use analysis" OFF)
		option(cucumber_cpp_runner_ENABLE_PCH "Enable precompiled headers" OFF)
	else ()
		option(cucumber_cpp_runner_ENABLE_TESTS "Enable test targets" ON)
		option(cucumber-cpp-runner-WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
		option(cucumber_cpp_runner_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
		option(cucumber_cpp_runner_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${SUPPORTS_ASAN})
		option(cucumber_cpp_runner_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
		option(cucumber_cpp_runner_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})
		option(cucumber_cpp_runner_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
		option(cucumber_cpp_runner_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
		option(cucumber_cpp_runner_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
		option(cucumber_cpp_runner_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
		option(cucumber_cpp_runner_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)
		option(cucumber_cpp_runner_ENABLE_INCLUDE_WHAT_YOU_USE "Enable include-what-you-use analysis" ON)
		option(cucumber_cpp_runner_ENABLE_PCH "Enable precompiled headers" OFF)
	endif ()

	if (NOT PROJECT_IS_TOP_LEVEL)
		mark_as_advanced(
			cucumber_cpp_runner_ENABLE_IPO
			cucumber-cpp-runner-WARNINGS_AS_ERRORS
			cucumber_cpp_runner_ENABLE_USER_LINKER
			cucumber_cpp_runner_ENABLE_SANITIZER_ADDRESS
			cucumber_cpp_runner_ENABLE_SANITIZER_LEAK
			cucumber_cpp_runner_ENABLE_SANITIZER_UNDEFINED
			cucumber_cpp_runner_ENABLE_SANITIZER_THREAD
			cucumber_cpp_runner_ENABLE_SANITIZER_MEMORY
			cucumber_cpp_runner_ENABLE_UNITY_BUILD
			cucumber_cpp_runner_ENABLE_CLANG_TIDY
			cucumber_cpp_runner_ENABLE_CPPCHECK
			cucumber_cpp_runner_ENABLE_INCLUDE_WHAT_YOU_USE
			cucumber_cpp_runner_ENABLE_COVERAGE
			cucumber_cpp_runner_ENABLE_PCH
			cucumber_cpp_runner_ENABLE_CACHE)
	endif ()

	cucumber_cpp_runner_check_libfuzzer_support(LIBFUZZER_SUPPORTED)
	if (LIBFUZZER_SUPPORTED AND (cucumber_cpp_runner_ENABLE_SANITIZER_ADDRESS OR cucumber_cpp_runner_ENABLE_SANITIZER_THREAD OR cucumber_cpp_runner_ENABLE_SANITIZER_UNDEFINED))
		set(DEFAULT_FUZZER ON)
	else ()
		set(DEFAULT_FUZZER OFF)
	endif ()
endmacro()

macro(cucumber_cpp_runner_global_options)
	if (cucumber_cpp_runner_ENABLE_IPO)
		include(cmake/InterproceduralOptimization.cmake)
		cucumber_cpp_runner_enable_ipo()
	endif ()

	cucumber_cpp_runner_supports_sanitizers()

	if (cucumber_cpp_runner_ENABLE_HARDENING AND cucumber_cpp_runner_ENABLE_GLOBAL_HARDENING)
		include(cmake/Hardening.cmake)
		if (NOT SUPPORTS_UBSAN
			OR cucumber_cpp_runner_ENABLE_SANITIZER_UNDEFINED
			OR cucumber_cpp_runner_ENABLE_SANITIZER_ADDRESS
			OR cucumber_cpp_runner_ENABLE_SANITIZER_THREAD
			OR cucumber_cpp_runner_ENABLE_SANITIZER_LEAK)
			set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
		else ()
			set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
		endif ()
		message("${cucumber_cpp_runner_ENABLE_HARDENING} ${ENABLE_UBSAN_MINIMAL_RUNTIME} ${cucumber_cpp_runner_ENABLE_SANITIZER_UNDEFINED}")
		cucumber_cpp_runner_enable_hardening(cucumber-cpp-runner-options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
	endif ()
endmacro()

macro(cucumber_cpp_runner_local_options)
	if (PROJECT_IS_TOP_LEVEL)
		include(cmake/StandardProjectSettings.cmake)
	endif ()

	add_library(cucumber-cpp-runner-warnings INTERFACE)
	add_library(cucumber-cpp-runner-options INTERFACE)

	include(cmake/CompilerWarnings.cmake)
	cucumber_cpp_runner_set_project_warnings(
		cucumber-cpp-runner-warnings
		${cucumber-cpp-runner-WARNINGS_AS_ERRORS}
		""
		""
		""
		"")

	if (cucumber_cpp_runner_ENABLE_USER_LINKER)
		include(cmake/Linker.cmake)
		configure_linker(cucumber-cpp-runner-options)
	endif ()

	include(cmake/Sanitizers.cmake)
	cucumber_cpp_runner_enable_sanitizers(
		cucumber-cpp-runner-options
		${cucumber_cpp_runner_ENABLE_SANITIZER_ADDRESS}
		${cucumber_cpp_runner_ENABLE_SANITIZER_LEAK}
		${cucumber_cpp_runner_ENABLE_SANITIZER_UNDEFINED}
		${cucumber_cpp_runner_ENABLE_SANITIZER_THREAD}
		${cucumber_cpp_runner_ENABLE_SANITIZER_MEMORY})

	set_target_properties(cucumber-cpp-runner-options PROPERTIES UNITY_BUILD ${cucumber_cpp_runner_ENABLE_UNITY_BUILD})

	if (cucumber_cpp_runner_ENABLE_PCH)
		target_precompile_headers(
			cucumber-cpp-runner-options
			INTERFACE
			<vector>
			<string>
			<utility>)
	endif ()

	if (cucumber_cpp_runner_ENABLE_CACHE)
		include(cmake/Cache.cmake)
		cucumber_cpp_runner_enable_cache()
	endif ()

	include(cmake/StaticAnalyzers.cmake)
	if (cucumber_cpp_runner_ENABLE_CLANG_TIDY)
		cucumber_cpp_runner_enable_clang_tidy(cucumber-cpp-runner-options ${cucumber-cpp-runner-WARNINGS_AS_ERRORS})
	endif ()

	if (cucumber_cpp_runner_ENABLE_CPPCHECK)
		cucumber_cpp_runner_enable_cppcheck(${cucumber-cpp-runner-WARNINGS_AS_ERRORS} "" # override cppcheck options
		)
	endif ()

	if (cucumber_cpp_runner_ENABLE_INCLUDE_WHAT_YOU_USE)
		cucumber_cpp_runner_enable_include_what_you_use()
	endif ()

	if (cucumber_cpp_runner_ENABLE_COVERAGE)
		include(cmake/Tests.cmake)
		cucumber_cpp_runner_enable_coverage(cucumber-cpp-runner-options)
	endif ()

	if (cucumber-cpp-runner-WARNINGS_AS_ERRORS)
		check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
		if (LINKER_FATAL_WARNINGS)
			# This is not working consistently, so disabling for now
			# target_link_options(cucumber-cpp-runner-options INTERFACE -Wl,--fatal-warnings)
		endif ()
	endif ()

	if (cucumber_cpp_runner_ENABLE_HARDENING AND NOT cucumber_cpp_runner_ENABLE_GLOBAL_HARDENING)
		include(cmake/Hardening.cmake)
		if (NOT SUPPORTS_UBSAN
			OR cucumber_cpp_runner_ENABLE_SANITIZER_UNDEFINED
			OR cucumber_cpp_runner_ENABLE_SANITIZER_ADDRESS
			OR cucumber_cpp_runner_ENABLE_SANITIZER_THREAD
			OR cucumber_cpp_runner_ENABLE_SANITIZER_LEAK)
			set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
		else ()
			set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
		endif ()
		cucumber_cpp_runner_enable_hardening(cucumber-cpp-runner-options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
	endif ()

endmacro()
