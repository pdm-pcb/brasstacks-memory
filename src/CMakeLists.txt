file(
    GLOB_RECURSE PROJECT_SOURCES
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

file(
    GLOB_RECURSE PROJECT_HEADERS
    "${PROJECT_SOURCE_DIR}/include/*.hpp"
)

configure_file(
    "${PROJECT_SOURCE_DIR}/src/version.hpp.in"
    "${PROJECT_SOURCE_DIR}/src/version.hpp"
)

target_sources(
    ${PROJECT_NAME}
    PRIVATE
        "${PROJECT_SOURCES}"
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${PROJECT_SOURCE_DIR}/include/"
        FILES "${PROJECT_HEADERS}"
)

include(FetchContent)
FetchContent_Declare(
    brasstacks_log EXCLUDE_FROM_ALL SYSTEM
    GIT_REPOSITORY https://github.com/pdm-pcb/brasstacks-log
    GIT_TAG main
)
FetchContent_MakeAvailable(brasstacks_log)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    brasstacks::log
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR
   CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        ${PROJECT_NAME} PRIVATE
        "-Wall"         # Enable most warnings
        "-Wextra"
        "-pedantic"     # and language compliance checks
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(
            ${PROJECT_NAME} PRIVATE
            "-Weverything"      # Clang supports enabling all possible warnings
            "-Wno-c++98-compat" # ...which means some must then be disabled
            "-Wno-c++98-compat-pedantic"
            "-Wno-newline-eof"

            "-Wno-unsafe-buffer-usage" # Pointer math is necessary
        )
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
        target_compile_options(
            ${PROJECT_NAME} PRIVATE
            "-march=x86-64-v3"    # Target Haswell/Excavator or newer
            "-mtune=x86-64-v3"
        )
    endif()

    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_options(
            ${PROJECT_NAME} PRIVATE
            "-O0"       # No optimization
            "-ggdb3"    # All the gdb flags
        )
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(
            ${PROJECT_NAME} PRIVATE
            "-O3"       # Optimize for speed, sacrifice size
        )
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(
        ${PROJECT_NAME} PRIVATE
        "/Wall"         # All warnings not disabled by default
    )

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
        target_compile_options(
            ${PROJECT_NAME} PRIVATE
            "/arch:AVX2"    # Target Haswell/Excavator or newer
        )
    endif()

    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_options(
            ${PROJECT_NAME} PRIVATE
            "/Od"       # Optimization disabled
            "/Zi"		# Generate .pdb files: implies /debug
            "/JMC"      # Just-My-Code: only step over user code
            "/RTCs"     # Runtime checks for stack frame behaviors
            "/RTCu"     # Runtime checks for uninitialized variables
            "/GS"       # Buffer overrun checks
            "/sdl"      # Compile and runtime security checks
            "/diagnostics:column" # Include column in compiler messages
        )

        target_link_options(
            ${PROJECT_NAME} PRIVATE
            "/DEBUG:FULL" # Move private symbol information into single .pdb
        )
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(
            ${PROJECT_NAME} PRIVATE
            "/O2"       # Optimize for speed. Implies:
                        # - global optimization
                        # - intrinsics
                        # - "favor fast code"
                        # - frame/pointer omission
                        # - inline function expansion
                        # - eliminate duplicate strings
                        # - function-level linking
            "/Ob3"      # More aggressive inline expansions than /O2
        )

        target_link_options(
            ${PROJECT_NAME} PRIVATE
            "/OPT:NOREF"    # Remove unused functions
        )
    endif()
endif()

set_target_properties(
	${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF

	ARCHIVE_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/debug/lib"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/debug/lib"

	ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/release/lib"
	LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/release/lib"
)