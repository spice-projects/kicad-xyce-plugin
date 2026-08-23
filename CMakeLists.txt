cmake_minimum_required(VERSION 3.21)

# on windows, build the app and all vcpkg dependencies with clang-cl (before project(), so both cmake and the vcpkg toolchain pick it up)
if(WIN32)
    # vcpkg community triplet that compiles dependencies with clang-cl instead of msvc
    set(VCPKG_TARGET_TRIPLET "x64-windows-clang-cl" CACHE STRING "vcpkg target triplet" FORCE)
    # default the project compilers to clang-cl, avoiding cmake picking the gnu-like clang driver from path
    if(NOT DEFINED CMAKE_CXX_COMPILER AND NOT DEFINED ENV{CXX} AND NOT DEFINED ENV{CC})
        find_program(KICAD_CLANG_CL_EXECUTABLE clang-cl)
        if(KICAD_CLANG_CL_EXECUTABLE)
            set(CMAKE_C_COMPILER clang-cl CACHE FILEPATH "windows c compiler" FORCE)
            set(CMAKE_CXX_COMPILER clang-cl CACHE FILEPATH "windows c++ compiler" FORCE)
        endif()
    endif()
endif()

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

find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nng CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)

if(APPLE)
    # slint style
    set(SLINT_STYLE "cupertino" CACHE STRING "Slint widget style" FORCE)
elseif (WIN32)
    # slint style
    set(SLINT_STYLE "fluent" CACHE STRING "Slint widget style" FORCE)
elseif (LINUX)
    # slint style
    set(SLINT_STYLE "cupertino" CACHE STRING "Slint widget style" FORCE)
endif()

# slint renderer
set(SLINT_FEATURE_RENDERER_SKIA ON CACHE BOOL "Include Skia renderer" FORCE)

# slint live-preview: debug builds can load/reload .slint files from disk at runtime
# (activate by setting SLINT_LIVE_PREVIEW=1 in the environment when launching the app);
# release/profile builds keep the ahead-of-time compiled designs
# slint mcp server: debug builds can be inspected/driven by AI agents over HTTP
# (activate by setting SLINT_MCP_PORT=<port> in the environment when launching the app)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SLINT_FEATURE_LIVE_PREVIEW ON CACHE BOOL "Enable Slint live-preview" FORCE)
    set(SLINT_FEATURE_MCP ON CACHE BOOL "Enable Slint embedded MCP server" FORCE)
else()
    set(SLINT_FEATURE_LIVE_PREVIEW OFF CACHE BOOL "Enable Slint live-preview" FORCE)
    set(SLINT_FEATURE_MCP OFF CACHE BOOL "Enable Slint embedded MCP server" FORCE)
endif()

# slint
include(FetchContent)
FetchContent_Declare(Slint GIT_REPOSITORY https://github.com/slint-ui/slint.git GIT_TAG release/1 SOURCE_SUBDIR api/cpp)
FetchContent_MakeAvailable(Slint)

# slint's generated internal headers trigger -Wgnu-anonymous-struct and -Wextra-semi
# under our warning set; collect slint's interface include directories here and expose
# them as system directories on the core library below, so warnings from slint headers
# are suppressed for the core library and every consumer of it.
# slint_target_sources additionally adds the build dir itself as a plain include
# directory for the generated widget headers (main_window.h, ...); those are generated
# code too, so treat them as system headers as well (e.g. the unused private
# 'live_preview' field emitted when live-preview is enabled)
set(SLINT_SYSTEM_INCLUDE_DIRS)
get_target_property(SLINT_INTERFACE_INCLUDES Slint INTERFACE_INCLUDE_DIRECTORIES)
foreach(include_dir IN LISTS SLINT_INTERFACE_INCLUDES)
    # keep build-interface paths only; install-interface paths never apply to this build
    if(include_dir MATCHES "^\\$<BUILD_INTERFACE:(.+)>$")
        list(APPEND SLINT_SYSTEM_INCLUDE_DIRS "${CMAKE_MATCH_1}")
    endif()
endforeach()
list(APPEND SLINT_SYSTEM_INCLUDE_DIRS "${CMAKE_CURRENT_BINARY_DIR}")

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
    # opengl is required on linux
    find_package(OpenGL REQUIRED)
    # gather linux sources
    file(GLOB_RECURSE PLATFORM_SOURCES "src/*.linux.c++")
    # combine remaining cpp sources with .mm sources
    set(SOURCE_FILES ${CPP_SOURCES} ${PLATFORM_SOURCES})
endif()

list(REMOVE_ITEM SOURCE_FILES "src/main.osx.mm" "src/main.win32.c++" "src/main.linux.c++")

# compile the generated proto sources into a library shared by the app and tests
add_library(kicad-proto STATIC ${KICAD_PROTO_GENERATED_SOURCES})

# expose the generated proto headers
target_include_directories(kicad-proto PUBLIC "${KICAD_PROTO_OUT_DIR}")
target_link_libraries(kicad-proto PUBLIC protobuf::libprotobuf)

# clang-cl (LLVM) raises -Winvalid-offsetof when compiling protobuf's generated
# TcParser tables, which reference private impl-members on non-standard-layout
# message classes. This is generated third-party code, so silence it for this
# target only under clang-cl on Windows.
if(MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(kicad-proto PRIVATE -Wno-invalid-offsetof)
endif()

# shared application code, compiled once and linked into both consumers
add_library(kicad-xyce-core STATIC ${SOURCE_FILES})
# expose the application header directory to consumers (executable and tests)
target_include_directories(kicad-xyce-core PUBLIC "src")
# link the core library with the vendored pocketfft library
target_include_directories(kicad-xyce-core PRIVATE ${POCKETFFT_INCLUDE_DIRS})
# expose slint's headers as system directories (see note above the slint fetch)
target_include_directories(kicad-xyce-core SYSTEM PUBLIC ${SLINT_SYSTEM_INCLUDE_DIRS})

# clang-cl treats -Wall as MSVC's /Wall (enable every warning), so only apply
# the standard warning set to GNU-style frontends (g++, clang++)
if(NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(kicad-xyce-core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# third-party dependencies, PUBLIC so both the executable and the tests transitively link the same set of libraries
target_link_libraries(kicad-xyce-core PUBLIC Slint::Slint imgui::imgui implot::implot spdlog::spdlog nng::nng kicad-proto)

if(APPLE)
    # link AppKit for NSApp dock icon and other cocoa apis
    target_link_libraries(kicad-xyce-core PUBLIC "-framework AppKit" "-framework Metal" "-framework MetalKit" "-framework GameController" "-framework UniformTypeIdentifiers" "-framework QuartzCore")
    # suppress duplicate library warnings on macOS, which are emitted when linking against the same framework multiple times (e.g. AppKit and Metal)
    target_link_options(kicad-xyce-core PUBLIC "-Wl,-no_warn_duplicate_libraries")
elseif(WIN32)
    # link windows graphics and system libraries required by the ui backend
    target_link_libraries(kicad-xyce-core PUBLIC d3d11 d3dcompiler dxgi gdi32 user32 shell32 ole32 oleaut32 uuid comdlg32 advapi32 version ws2_32 wininet oleacc uxtheme)
elseif(LINUX)
    target_link_libraries(kicad-xyce-core PUBLIC OpenGL::GL)
endif()

# slint: embed images/fonts referenced with @image-url() into the binary
set_property(TARGET kicad-xyce-core PROPERTY SLINT_EMBED_RESOURCES embed-files)

# Only root widgets (windows/dialogs) are compiled as independent Slint roots.
# Shared components under src/ui/components/ are purely imported by these roots
# and are never compiled standalone, so they are excluded here. Each root is
# compiled into its own C++ namespace: the Slint compiler emits the embedded
# image/font tables (slint_embedded_resource_*) with the same global symbol
# names in every root, so distinct namespaces prevent duplicate symbols.
file(GLOB_RECURSE SLINT_WIDGET_SOURCES "src/ui/widgets/*.slint")
foreach(widget_source IN LISTS SLINT_WIDGET_SOURCES)
    get_filename_component(widget_namespace ${widget_source} NAME_WE)
    slint_target_sources(kicad-xyce-core ${widget_source} NAMESPACE ${widget_namespace})
    # the generated translation unit lives in the build dir next to the generated
    # headers, so it picks them up through the quoted-include path before any
    # -isystem entry and compiles them with our full warning set; silence all
    # warnings for this fully generated code instead
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/slint_generated_${widget_namespace}_1.cpp"
        PROPERTIES COMPILE_OPTIONS "-w")
endforeach()

if(APPLE)
#     set_source_files_properties("plugin-icon-64x64.png" PROPERTIES MACOSX_BUNDLE_OUTPUT_DIRECTORY "Resources")

#     # create bundle
#     add_executable(kicad-xyce-plugin MACOSX_BUNDLE src/main.osx.mm)

#     set_target_properties(kicad-xyce-plugin PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KiCad Xyce Plugin" MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.spice-projects.kicad-xyce-plugin")

    # declare application
    add_executable(kicad-xyce-plugin src/main.osx.mm)
elseif(WIN32)
    # declare application
    add_executable(kicad-xyce-plugin src/main.win32.c++)
elseif(LINUX)
    # declare application
    add_executable(kicad-xyce-plugin src/main.linux.c++)
endif()

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
