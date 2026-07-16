cmake_minimum_required(VERSION 3.12)
project(kicad-xyce-plugin LANGUAGES CXX)

# c++ 23
set(CMAKE_CXX_STANDARD 23)
# enforce the c++23 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# disable compiler specific things
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    enable_language(OBJCXX)
endif()

option(BUILD_TESTS "Build tests" ON)

set(wxBUILD_SAMPLES OFF)
set(wxBUILD_SHARED OFF)

find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(wxwidgets CONFIG REQUIRED)

# gather cpp sources (all platforms)
file(GLOB_RECURSE CPP_SOURCES "src/*.cpp" "src/expression/*.cpp" "src/file/*.cpp" "src/ui/*.cpp")

if(APPLE)
    # gather objective-c++ sources
    file(GLOB_RECURSE MM_SOURCES "src/*.mm" "src/ui/*.mm")
    # combine remaining cpp sources with .mm sources
    set(SOURCE_FILES ${CPP_SOURCES} ${MM_SOURCES})
else()
    # non-apple builds use only cpp sources
    set(SOURCE_FILES ${CPP_SOURCES})
endif()

add_executable(kicad-xyce-plugin ${SOURCE_FILES})

target_link_libraries(kicad-xyce-plugin PRIVATE wx::core wx::base imgui::imgui implot::implot)

if(APPLE)
    # link AppKit for NSApp dock icon and other cocoa apis
    target_link_libraries(kicad-xyce-plugin PRIVATE "-framework Foundation" "-framework AppKit" "-framework Metal" "-framework MetalKit" "-framework GameController")
endif()

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
