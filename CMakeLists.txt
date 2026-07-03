# Specify the minimum CMake version required
cmake_minimum_required(VERSION 3.15)

# Set the project name and programming language
project(WinSerial VERSION 1.0.0 LANGUAGES CXX)

# Enable Unicode for Windows API to resolve C2664 compilation error
add_compile_definitions(UNICODE _UNICODE)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find the required Boost libraries
find_package(Boost CONFIG REQUIRED)

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

# Add Boost include directories
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
