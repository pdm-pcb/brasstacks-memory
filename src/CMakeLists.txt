file(
    GLOB_RECURSE LIBRARY_SOURCE
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)
file(
    GLOB_RECURSE LIBRARY_HEADERS
    "${CMAKE_SOURCE_DIR}/include/brasstacks_memory/*.hpp"
)

set(LIBRARY_TARGET brasstacks_memory)

add_library(
    ${LIBRARY_TARGET} STATIC
	${LIBRARY_SOURCE}
    ${LIBRARY_HEADERS}
)

target_include_directories(
    ${LIBRARY_TARGET} PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    target_compile_options(
        ${LIBRARY_TARGET} PUBLIC
        "-Wall" # All, extra, and pedantic warnings
        "-Wextra"
        "-pedantic"
    )

    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_options(
            ${LIBRARY_TARGET} PUBLIC
            "-O0"       # No optimization
            "-ggdb3"    # All the gdb flags
        )
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(
            ${LIBRARY_TARGET} PUBLIC
            "-O3"           # Healthy optimization
            "-ffast-math"   # Gotta go fast
        )
    endif()
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    if(CMAKE_C_COMPILER MATCHES "clang")
        target_compile_options(
            ${LIBRARY_TARGET} PUBLIC
            "-Wall" # All, extra, and pedantic warnings
            "-Wextra"
            "-pedantic"
        )

        if(CMAKE_BUILD_TYPE MATCHES "Debug")
            target_compile_options(
                ${LIBRARY_TARGET} PUBLIC
                "-O0"       # No optimization
                "-ggdb3"    # All the gdb flags
            )
        elseif(CMAKE_BUILD_TYPE MATCHES "Release")
            target_compile_options(
                ${LIBRARY_TARGET} PUBLIC
                "-O3"           # Healthy optimization
                "-ffast-math"   # Gotta go fast
            )
        endif()
    else()
        target_compile_definitions(
            ${LIBRARY_TARGET} PUBLIC
            "_CRT_SECURE_NO_WARNINGS" # Ignore "may be unsafe" warnings
            "WIN64"                   # Prefer 64 bit address space
        )

        target_compile_options(
            ${LIBRARY_TARGET} PUBLIC
            "/nologo"  # Suppress copyright message
            "/W4"	   # Warnin's
            "/wd4312"  # Disable the warning for casting to a larger size
            "/wd4324"  # Disable the warning for padding due to alignas
        )

        if(CMAKE_BUILD_TYPE MATCHES "Debug")
            target_compile_options(
                ${LIBRARY_TARGET} PUBLIC
                "/JMC"      # Just-My-Code
                "/Zi"		# Debugging information
                "/RTC1"     # Runtime checks
                "/GS"       # Buffer overrun checks
                "/sdl"      # Security warnings
                "/Od"       # Optimization disabled
                "/diagnostics:column" # Include column in compiler messages
            )
        elseif(CMAKE_BUILD_TYPE MATCHES "Release")
            target_compile_options(
                ${LIBRARY_TARGET} PUBLIC
                "/O2"         # Optimization not disabled =D
                "/fp:fast"    # Gotta go fast
                "/GS-"        # No buffer overrun checks
            )
        endif()
    endif()
endif()

set_target_properties(
	${LIBRARY_TARGET} PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF

	ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_SOURCE_DIR}/debug/lib
    LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_SOURCE_DIR}/debug/lib

	ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/release/lib
	LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/release/lib
)