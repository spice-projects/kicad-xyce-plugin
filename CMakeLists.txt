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

# do not use precompiled headers (keeps wx/wxprec.h include blocks active)
add_compile_definitions(NOPCH)

# on Windows, prevent winsock.h (WinSock 1.1) from being auto-included by
# windows.h, which conflicts with winsock2.h pulled in by wxWidgets headers
if(WIN32)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
endif()

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
find_package(nng CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)

# generate C++ code from the vendored KiCad API protos
set(KICAD_PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/kicad/proto")
set(KICAD_PROTO_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/kicad-proto")
protobuf_generate(
    PROTOS
        "${KICAD_PROTO_DIR}/common/envelope.proto"
        "${KICAD_PROTO_DIR}/common/commands/base_commands.proto"
        "${KICAD_PROTO_DIR}/common/types/base_types.proto"
    IMPORT_DIRS "${KICAD_PROTO_DIR}"
    PROTOC_OUT_DIR "${KICAD_PROTO_OUT_DIR}"
    OUT_VAR KICAD_PROTO_GENERATED_SOURCES
)

# gather cpp sources (all platforms)
file(GLOB_RECURSE CPP_SOURCES "src/*.cpp")

# header only library: pocketfft
find_path(POCKETFFT_INCLUDE_DIRS "pocketfft_hdronly.h")

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
    # opengl is required for the wxGLCanvas backend on linux
    find_package(OpenGL REQUIRED)
    # gather linux sources
    file(GLOB_RECURSE PLATFORM_SOURCES "src/*.linux.c++")
    # combine remaining cpp sources with .mm sources
    set(SOURCE_FILES ${CPP_SOURCES} ${PLATFORM_SOURCES})
endif()

# The executable only needs to own the wxApp entry point (src/main.cpp); all
# other sources are shared with the test runner. Compile them into a single
# static library so each file is compiled exactly once, then link both the
# executable and the tests against it.
list(REMOVE_ITEM SOURCE_FILES "src/main.cpp")

# compile the generated proto sources into a library shared by the app and tests
add_library(kicad-proto STATIC ${KICAD_PROTO_GENERATED_SOURCES})

# expose the generated proto headers
target_include_directories(kicad-proto PUBLIC "${KICAD_PROTO_OUT_DIR}")
target_link_libraries(kicad-proto PUBLIC protobuf::libprotobuf)

# shared application code, compiled once and linked into both consumers
add_library(kicad-xyce-core STATIC ${SOURCE_FILES})
# expose the application header directory to consumers (executable and tests)
target_include_directories(kicad-xyce-core PUBLIC "src")
# link the core library with the vendored pocketfft library
target_include_directories(kicad-xyce-core PRIVATE ${POCKETFFT_INCLUDE_DIRS})
target_compile_definitions(kicad-xyce-core PUBLIC KICAD_XYCE_FONTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src/ui/fonts")

# clang-cl treats -Wall as MSVC's /Wall (enable every warning), so only apply
# the standard warning set to GNU-style frontends (g++, clang++)
if(NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(kicad-xyce-core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# third-party dependencies, PUBLIC so both the executable and the tests transitively link the same set of libraries
target_link_libraries(kicad-xyce-core PUBLIC wx::core wx::base wx::stc imgui::imgui implot::implot spdlog::spdlog nng::nng protobuf::libprotobuf kicad-proto)

if(APPLE)
    # link AppKit for NSApp dock icon and other cocoa apis
    target_link_libraries(kicad-xyce-core PUBLIC "-framework AppKit" "-framework Metal" "-framework MetalKit" "-framework GameController")
    # suppress duplicate library warnings on macOS, which are emitted when linking against the same framework multiple times (e.g. AppKit and Metal)
    target_link_options(kicad-xyce-core PUBLIC "-Wl,-no_warn_duplicate_libraries")
elseif(WIN32)
    # link windows graphics and system libraries required by the ui backend
    target_link_libraries(kicad-xyce-core PUBLIC d3d11 d3dcompiler dxgi gdi32 user32 shell32 ole32 oleaut32 uuid comdlg32 advapi32 version ws2_32 wininet oleacc uxtheme)
elseif(LINUX)
    target_link_libraries(kicad-xyce-core PUBLIC wx::gl OpenGL::GL)
endif()

# the application executable owns only the wxApp entry point
add_executable(kicad-xyce-plugin src/main.cpp)
target_link_libraries(kicad-xyce-plugin PRIVATE kicad-xyce-core)

if(WIN32)
    set_target_properties(kicad-xyce-plugin PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# profile build type, activate with `cmake --preset debug -DCMAKE_BUILD_TYPE=Profile` && `cmake --build --preset debug`
set(CMAKE_CXX_FLAGS_PROFILE "-O2 -fprofile-instr-generate -fcoverage-mapping")
set(CMAKE_EXE_LINKER_FLAGS_PROFILE "-fprofile-instr-generate")

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
