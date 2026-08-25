cmake_minimum_required(VERSION 3.21)

project(kicad-xyce-plugin-tests LANGUAGES CXX)

# c++ 23
set(CMAKE_CXX_STANDARD 23)
# enforce the c++23 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# disable compiler specific things
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(gtest CONFIG REQUIRED)

file(GLOB_RECURSE TEST_FILES "./*.cpp")

message(STATUS "Collected test files:\n${TEST_FILES}")

# the test runner tests against the shared core library, so the shared application sources are not compiled again for this target
add_executable(kicad-xyce-plugin-tests ${TEST_FILES})
# depend on the generated proto sources before building
add_dependencies(kicad-xyce-plugin-tests kicad-proto)

# link the core library (which brings the app headers, defines, and the slint/imgui/implot/spdlog/nng/protobuf/platform dependencies), plus gtest
target_link_libraries(kicad-xyce-plugin-tests PRIVATE kicad-xyce-core GTest::gtest_main)

# enable more warnings in tests for better clang-cl treats -Wall as MSVC's /Wall (enable every warning), so only apply the standard warning set to GNU-style frontends (clang++, g++)
if(NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(kicad-xyce-plugin-tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32 AND TARGET slint_cpp-shared)
    # copy the slint shared library to the output path
    add_custom_command(TARGET kicad-xyce-plugin-tests POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:slint_cpp-shared> $<TARGET_FILE_DIR:kicad-xyce-plugin-tests>)
endif()

# auto-discover Gtests as Ctests
include(CTest)
include(GoogleTest)
gtest_discover_tests(kicad-xyce-plugin-tests)
# add_test(NAME AllGoogleTests COMMAND kicad-xyce-plugin-tests)
