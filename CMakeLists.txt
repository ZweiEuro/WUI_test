cmake_minimum_required(VERSION 3.14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

project("OffscreenUI_test" CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)




#source files
file(GLOB_RECURSE sources src/*.cpp)
add_executable(${PROJECT_NAME} ${sources})

#lib files
add_subdirectory(libs)

#header files 
target_include_directories(${PROJECT_NAME} PRIVATE include/)


# Libraries
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE 
  allegro
  allegro_image
  allegro_primitives
  allegro_ttf
  allegro_font
  allegro_dialog
  Threads::Threads)

