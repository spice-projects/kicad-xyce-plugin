cmake_minimum_required(VERSION 3.12)
if(APPLE)
    project(kicad-xyce-plugin LANGUAGES CXX OBJCXX)
else()
    project(kicad-xyce-plugin LANGUAGES CXX)
endif()

# c++ 23
set(CMAKE_CXX_STANDARD 23)
# enforce the c++23 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# disable compiler specific things
set(CMAKE_CXX_EXTENSIONS OFF)

# generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
    # enable objective-c++ language for .mm files
    enable_language(OBJCXX)
    # c++ 23 (mm files)
    set(CMAKE_OBJCXX_STANDARD 23)
    # enforce the c++23 standard
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    # disable compiler specific things
    set(CMAKE_OBJCXX_EXTENSIONS OFF)
endif()

option(BUILD_TESTS "Build tests" ON)

set(wxBUILD_SAMPLES OFF)
set(wxBUILD_SHARED OFF)

find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(wxwidgets CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# gather cpp sources (all platforms)
file(GLOB_RECURSE CPP_SOURCES "src/*.cpp")

# include should be relative to src
include_directories("src")

if(APPLE)
    # gather objective-c++ sources
    file(GLOB_RECURSE PLATFORM_SOURCES "src/*.osx.mm")
    # combine remaining cpp sources with .mm sources
    set(SOURCE_FILES ${CPP_SOURCES} ${PLATFORM_SOURCES})
elseif (WIN32)
    # gather win32 sources
    file(GLOB_RECURSE PLATFORM_SOURCES "src/*.win32.c++")
    # combine remaining cpp sources with .mm sources
    set(SOURCE_FILES ${CPP_SOURCES} ${PLATFORM_SOURCES})
elseif (LINUX)
    # gather linux sources
    file(GLOB_RECURSE PLATFORM_SOURCES "src/*.linux.c++")
    # combine remaining cpp sources with .mm sources
    set(SOURCE_FILES ${CPP_SOURCES} ${PLATFORM_SOURCES})
endif()

add_executable(kicad-xyce-plugin ${SOURCE_FILES})

if(WIN32)
    set_target_properties(kicad-xyce-plugin PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# profile build type, activate with `cmake --preset debug -DCMAKE_BUILD_TYPE=Profile` && `cmake --build --preset debug`
set(CMAKE_CXX_FLAGS_PROFILE "-O2 -fprofile-instr-generate -fcoverage-mapping")
set(CMAKE_EXE_LINKER_FLAGS_PROFILE "-fprofile-instr-generate")

target_compile_definitions(kicad-xyce-plugin PRIVATE KICAD_XYCE_FONTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src/ui/fonts")

target_compile_options(kicad-xyce-plugin PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(kicad-xyce-plugin PRIVATE wx::core wx::base wx::stc imgui::imgui implot::implot spdlog::spdlog)

if(APPLE)
    # link AppKit for NSApp dock icon and other cocoa apis
    target_link_libraries(kicad-xyce-plugin PRIVATE "-framework AppKit" "-framework Metal" "-framework MetalKit" "-framework GameController")
    target_link_options(kicad-xyce-plugin PRIVATE "-Wl,-no_warn_duplicate_libraries")
elseif(WIN32)
    # link windows graphics and system libraries required by the ui backend
    target_link_libraries(kicad-xyce-plugin PRIVATE d3d11 d3dcompiler dxgi gdi32 user32 shell32 ole32 oleaut32 uuid comdlg32 advapi32 version ws2_32 wininet oleacc uxtheme)
endif()

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
