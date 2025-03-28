cmake_minimum_required(VERSION 3.10)
project(rstk-refactor)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable compile_commands.json generation
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find readline package (do this once at the top level)
find_library(READLINE_LIBRARY NAMES readline REQUIRED)
find_library(HISTORY_LIBRARY NAMES history REQUIRED)

if(NOT READLINE_LIBRARY OR NOT HISTORY_LIBRARY)
    message(FATAL_ERROR "readline or history library not found")
endif()

# Collect all source files
file(GLOB_RECURSE SOURCES "src/**/*.cpp" "main.cpp")

# Create the main executable
add_executable(rstk-refactor ${SOURCES})

# Specify include directories for the main executable
target_include_directories(rstk-refactor PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Link readline to main executable
target_link_libraries(rstk-refactor
    PRIVATE
    ${READLINE_LIBRARY}
    ${HISTORY_LIBRARY}
)

# Add Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/b514bdc898e2951020cbdca1304b75f5950d1f59.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Collect test sources
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
file(GLOB_RECURSE SRC_SOURCES "src/**/*.cpp")

# Create test executable
add_executable(rstk-tests ${TEST_SOURCES} ${SRC_SOURCES})

# Specify include directories for tests
target_include_directories(rstk-tests PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Link Google Test and readline to test executable
target_link_libraries(rstk-tests
    PRIVATE
    gtest
    gtest_main
    ${READLINE_LIBRARY}
    ${HISTORY_LIBRARY}
)

# Add test
add_test(NAME RunAllTests COMMAND rstk-tests)
