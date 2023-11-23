include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG 3f0283de7a9c43200033da996ff9093be3ac84dc # 2023-02-27
    FIND_PACKAGE_ARGS 3.3.2
)
FetchContent_MakeAvailable(Catch2)

set(TEST_TARGET ${PROJECT_NAME}_test)
add_executable(${TEST_TARGET})

file(
    GLOB_RECURSE TEST_SOURCES
    "${PROJECT_SOURCE_DIR}/test/*.cpp"
)

file(
    GLOB_RECURSE TEST_HEADERS
    "${PROJECT_SOURCE_DIR}/test/*.hpp"
)

target_sources(
    ${TEST_TARGET}
    PRIVATE
        "${TEST_SOURCES}"
        "${TEST_HEADERS}"
)

target_link_libraries(
    ${TEST_TARGET} PUBLIC
	brasstacks::memory
    Catch2::Catch2WithMain
)

set_target_properties(
    ${TEST_TARGET} PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF

	RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_SOURCE_DIR}/debug/bin
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/release/bin
)