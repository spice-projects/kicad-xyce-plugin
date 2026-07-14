cmake_minimum_required(VERSION 3.12)
project(kicad-xyce-plugin-tests LANGUAGES CXX)

# c++ 23
set(CMAKE_CXX_STANDARD 23)
# enforce the c++23 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# disable compiler specific things
set(CMAKE_CXX_EXTENSIONS OFF)

# include package that lets us get things from git
include(FetchContent)

# get google test from git
FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.17.0)

# make it available for use in this CMakeLists.txt
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE SOURCE_FILES "../src/*.cpp")
file(GLOB_RECURSE TEST_FILES "./*.cpp")

include_directories(../src)

# math-tests executable
add_executable(kicad-xyce-plugin-tests ${TEST_FILES} ${SOURCE_FILES})

# link
target_link_libraries(kicad-xyce-plugin-tests PRIVATE GTest::gtest_main imgui imgui-backend)

if(APPLE)
    target_link_libraries(kicad-xyce-plugin-tests PRIVATE
            "-framework Foundation"
            "-framework AppKit"
            "-framework Metal"
            "-framework MetalKit"
            "-framework GameController"
    )
endif()

# enable more warnings in tests for better
target_compile_options(kicad-xyce-plugin-tests PRIVATE -Wall -Wextra -Wpedantic)

# auto-discover Gtests as Ctests
include(GoogleTest)
gtest_discover_tests(kicad-xyce-plugin-tests)
