cmake_minimum_required(VERSION 3.12)
project(kicad-xyce-plugin LANGUAGES CXX)

# c++ 23
set(CMAKE_CXX_STANDARD 23)
# enforce the c++23 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# disable compiler specific things
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build tests" ON)

add_subdirectory(dependencies/imgui)
include_directories(dependencies/imgui)

# find all cpp files
file(GLOB_RECURSE SOURCE_FILES "src/*.mm" "src/*.cpp")

add_executable(kicad-xyce-plugin ${SOURCE_FILES})

target_link_libraries(kicad-xyce-plugin PRIVATE imgui imgui-backend)

if(APPLE)
    target_link_libraries(kicad-xyce-plugin PRIVATE
            "-framework Foundation"
            "-framework AppKit"
            "-framework Metal"
            "-framework MetalKit"
            "-framework GameController"
    )
endif()

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
