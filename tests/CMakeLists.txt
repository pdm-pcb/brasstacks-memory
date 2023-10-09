include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG 3f0283de7a9c43200033da996ff9093be3ac84dc # 2023-02-27
    FIND_PACKAGE_ARGS 3.3.2
)

FetchContent_MakeAvailable(Catch2)

file(
    GLOB_RECURSE TESTS_SOURCE
    "*.cpp"
)

file(
    GLOB_RECURSE TESTS_HEADERS
    "*.hpp"
)

add_executable(
    brasstacks-memory-tests
	${TESTS_SOURCE}
	${TESTS_HEADERS}
)

target_include_directories(
    brasstacks-memory-tests PUBLIC
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(
	brasstacks-memory-tests PUBLIC
	brasstacks_memory
    Catch2::Catch2WithMain
)

set_target_properties(
	brasstacks-memory-tests PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON

	RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_SOURCE_DIR}/debug/bin
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/release/bin
)