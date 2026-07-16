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

add_subdirectory(vendor/wxWidgets)
add_subdirectory(dependencies/imgui)
include_directories(dependencies/imgui)

# gather cpp sources (all platforms)
file(GLOB_RECURSE CPP_SOURCES "src/*.cpp" "src/expression/*.cpp" "src/file/*.cpp" "src/ui/*.cpp")

if(APPLE)
    # gather objective-c++ sources
    file(GLOB_RECURSE MM_SOURCES "src/*.mm")
    # remove any cpp file that has a .mm counterpart — the .mm replaces it on apple
    foreach(MM_FILE ${MM_SOURCES})
        string(REPLACE ".mm" ".cpp" CPP_EQUIV ${MM_FILE})
        list(REMOVE_ITEM CPP_SOURCES ${CPP_EQUIV})
    endforeach()
    # combine remaining cpp sources with .mm sources
    set(SOURCE_FILES ${CPP_SOURCES} ${MM_SOURCES})
else()
    # non-apple builds use only cpp sources
    set(SOURCE_FILES ${CPP_SOURCES})
endif()

add_executable(kicad-xyce-plugin ${SOURCE_FILES})

target_link_libraries(kicad-xyce-plugin PRIVATE wx::core wx::base imgui imgui-backend)

if(APPLE)
    # link AppKit for NSApp dock icon and other cocoa apis
    target_link_libraries(kicad-xyce-plugin PRIVATE "-framework AppKit")
    # suppress deprecation warnings from wxWidgets vendor headers (NSOpenGLContext, NSOpenGLPixelFormat)
    # these are in the vendor directory and cannot be modified; the warnings are harmless
    target_compile_options(kicad-xyce-plugin PRIVATE -Wno-deprecated-declarations)
endif()

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
