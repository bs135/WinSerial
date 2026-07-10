# Minimum CMake version required
cmake_minimum_required(VERSION 3.15)

# Project metadata
project(WinSerial VERSION 1.7.2 LANGUAGES CXX)

# Enable Unicode for Windows API to resolve C2664 compilation errors
add_compile_definitions(UNICODE _UNICODE)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find dependencies
find_package(Boost CONFIG REQUIRED)
find_package(Git QUIET)

# Retrieve current git commit hash
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE PROJECT_VERSION_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
    set(PROJECT_VERSION_COMMIT "unknown")
endif()

message(STATUS "Git Commit Hash: ${PROJECT_VERSION_COMMIT}")

# Generate version header from template
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Version.h"
    @ONLY
)

# Include generated header directory
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# List project source files
set(SOURCES
    WinSerial.cpp
    WinSerial.h
    framework.h
    resource.h
    targetver.h
    WinSerial.rc
)

# Define the executable target
add_executable(WinSerial ${SOURCES})

# Configure include directories
target_include_directories(WinSerial PRIVATE ${Boost_INCLUDE_DIRS})

# Link necessary Windows system libraries
# - advapi32: Windows Registry operations (e.g., RegOpenKeyEx)
# - user32: Window management functions (e.g., CenterParentWindow, GetDlgItem)
# - comctl32: Common Controls for advanced UI elements (e.g., ComboBox)
target_link_libraries(WinSerial PRIVATE
    Boost::boost
    advapi32
    user32
    comctl32
)
