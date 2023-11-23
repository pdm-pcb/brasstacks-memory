cmake_minimum_required(VERSION 3.23) # Required for File Sets

project(
    brasstacks_memory
    VERSION 0.1.0
    DESCRIPTION "A simple heap allocator for learning and video games."
    LANGUAGES CXX
)

add_library(${PROJECT_NAME} STATIC)
add_library(brasstacks::memory ALIAS ${PROJECT_NAME})

add_subdirectory(src)

# Only touch test when we're not being pulled in by something else
if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(test)
endif()