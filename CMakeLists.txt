cmake_minimum_required(VERSION 3.20) # This minimum requirement allows us to not
                                     # worry whether CMake is automatically
                                     # adding the RTTI enable flag to an MSVC
                                     # build

project(
    brasstacks_memory
    VERSION 0.1.0
    DESCRIPTION "Simple memory management for learning and video games"
    LANGUAGES CXX C
)

add_subdirectory(src)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    add_subdirectory(tests)
endif()