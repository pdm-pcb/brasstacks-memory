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

set(BINARY_TARGET brasstacks_memory_tests)

add_executable(
    ${BINARY_TARGET}
	${TESTS_SOURCE}
	${TESTS_HEADERS}
)

target_include_directories(
    ${BINARY_TARGET} PUBLIC
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(
    ${BINARY_TARGET} PUBLIC
	brasstacks_memory
    Catch2::Catch2WithMain
)

set_target_properties(
    ${BINARY_TARGET} PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF

	RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_SOURCE_DIR}/debug/bin
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/release/bin
)