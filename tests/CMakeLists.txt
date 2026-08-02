cmake_minimum_required(VERSION 3.12)
project(kicad-xyce-plugin-tests LANGUAGES CXX)

# c++ 23
set(CMAKE_CXX_STANDARD 23)
# enforce the c++23 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# disable compiler specific things
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    enable_language(OBJCXX)
endif()

find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(wxwidgets CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(gtest CONFIG REQUIRED)

file(GLOB_RECURSE CPP_SOURCES "../src/step_information.cpp" "../src/util.cpp" "../src/config/*.cpp" "../src/expression/*.cpp" "../src/file/*.cpp" "../src/netlist/*.cpp" "../src/simulation_parameters/*.cpp" "../src/ui/*.cpp")
file(GLOB_RECURSE TEST_FILES "./*.cpp")

include_directories("../src")

if(APPLE)
    # gather objective-c++ sources
    file(GLOB_RECURSE PLATFORM_SOURCES "../src/*.osx.mm" "../src/ui/*.osx.mm")
    # combine remaining cpp sources with .mm sources
    set(SOURCE_FILES ${CPP_SOURCES} ${PLATFORM_SOURCES})
elseif (WINDOWS)
    # gather win32 sources
    file(GLOB_RECURSE PLATFORM_SOURCES "../src/*.win32.c++" "../src/ui/*.win32.c++")
    # combine remaining cpp sources with .mm sources
    set(SOURCE_FILES ${CPP_SOURCES} ${PLATFORM_SOURCES})
elseif (LINUX)
    # gather linux sources
    file(GLOB_RECURSE PLATFORM_SOURCES "../src/*.linux.c++" "../src/ui/*.linux.c++")
    # combine remaining cpp sources with .mm sources
    set(SOURCE_FILES ${CPP_SOURCES} ${PLATFORM_SOURCES})
endif()

message(STATUS "Collected source files:\n${CPP_SOURCES}")

add_executable(kicad-xyce-plugin-tests ${TEST_FILES} ${SOURCE_FILES})

target_compile_definitions(kicad-xyce-plugin-tests PRIVATE KICAD_XYCE_FONTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src/ui/fonts")

target_link_libraries(kicad-xyce-plugin-tests PRIVATE wx::core wx::base wx::stc imgui::imgui implot::implot spdlog::spdlog GTest::gtest_main)

if(APPLE)
    # link AppKit for NSApp dock icon and other cocoa apis
    target_link_libraries(kicad-xyce-plugin-tests PRIVATE "-framework AppKit" "-framework Metal" "-framework MetalKit" "-framework GameController")
    target_link_options(kicad-xyce-plugin-tests PRIVATE "-Wl,-no_warn_duplicate_libraries")
elseif(WIN32)
    # link windows graphics and system libraries required by the ui backend
    target_link_libraries(kicad-xyce-plugin-tests PRIVATE d3d11 d3dcompiler dxgi gdi32 user32 shell32 ole32 oleaut32 uuid comdlg32 advapi32 version ws2_32 wininet oleacc uxtheme)
endif()

# enable more warnings in tests for better
# clang-cl treats -Wall as MSVC's /Wall (enable every warning), so only apply
# the standard warning set to GNU-style frontends (clang++, g++)
if(NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(kicad-xyce-plugin-tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# auto-discover Gtests as Ctests
include(CTest)
include(GoogleTest)
gtest_discover_tests(kicad-xyce-plugin-tests)
# add_test(NAME AllGoogleTests COMMAND kicad-xyce-plugin-tests)
