#
# Global setup.
#

# For VS2019 and Xcode 12+ support.
cmake_minimum_required(VERSION 3.19)

# Only generate Debug and Release configuration types.
set(CMAKE_CONFIGURATION_TYPES Debug Release)

# Project name.
project(WebUI)

# Use folders in the resulting project files.
set_property(GLOBAL PROPERTY OS_FOLDERS ON)

# Example 3: The binary distribution is in a separate directory from your
#            project. Locate the binary distribution using the CEF_ROOT
#            environment variable.
# A. Create a directory structure for your project like the following:
#    myproject/
#      CMakeLists.txt    <= top-level CMake configuration
#      cmake/
#        FindCEF.cmake   <= CEF CMake configuration entry point
#      mytarget/
#        CMakeLists.txt  <= CMake configuration for `mytarget`
#        ... other `mytarget` source files
# B. Copy this file to "myproject/CMakeLists.txt" as the top-level CMake
#    configuration.
# C. Copy the cmake/FindCEF.cmake file to "myproject/cmake/FindCEF.cmake".
# D. Create the target-specific "myproject/mytarget/CMakeLists.txt" file for
#    your application. See the included cefclient and cefsimple CMakeLists.txt
#    files as an example.
# E. Set the CEF_ROOT environment variable before executing CMake. For example:
#    > set CEF_ROOT=c:\path\to\cef_binary_3.2704.xxxx.gyyyyyyy_windows32
# F. Comment in these lines:

set(CEF_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/CEF")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


#
# Load the CEF configuration.
#

# Execute FindCEF.cmake which must exist in CMAKE_MODULE_PATH.
find_package(CEF REQUIRED)


#
# Define CEF-based targets.
#

# Include the libcef_dll_wrapper target.
# Comes from the libcef_dll/CMakeLists.txt file in the binary distribution
# directory.
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)
add_subdirectory(webUI)

# Display configuration settings.
PRINT_CEF_CONFIG()


