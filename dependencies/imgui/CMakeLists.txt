cmake_minimum_required(VERSION 3.12)
project(imgui LANGUAGES CXX)

# c++ 23
set(CMAKE_CXX_STANDARD 23)
# enforce the c++23 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# disable compiler specific things
set(CMAKE_CXX_EXTENSIONS OFF)

# source files
file(GLOB_RECURSE IMGUI_SOURCE_FILES "*.cpp")

if(APPLE)
    # Mac specific backend files
    file(GLOB_RECURSE IMGUI_BACKEND_SOURCE_FILES "*.mm")
endif()

# imgui
add_library(imgui STATIC ${IMGUI_SOURCE_FILES})

# backend
add_library(imgui-backend STATIC ${IMGUI_BACKEND_SOURCE_FILES})
