cmake_minimum_required(VERSION 3.21)

project(spike-skia LANGUAGES CXX)

# c++ 23
set(CMAKE_CXX_STANDARD 23)
# enforce the c++23 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# disable compiler specific things
set(CMAKE_CXX_EXTENSIONS OFF)

# throwaway phase-0 probe: links vcpkg skia and slint (which vendors its own
# skia internally) into ONE binary, renders a raster pattern offscreen and
# shows it as a slint image; retires risks R2/R3/R4 of skia-imgui-implot.md
add_executable(spike-skia spike.cpp)

# generate C++ code from the spike design
slint_target_sources(spike-skia spike.slint NAMESPACE spike)
# silence all warnings for this fully generated code
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/slint_generated_spike_1.cpp" PROPERTIES COMPILE_OPTIONS "-w")

# expose slint's headers as system directories (same treatment as the core library)
target_include_directories(spike-skia SYSTEM PRIVATE ${SLINT_SYSTEM_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(spike-skia PRIVATE unofficial::skia::skia Slint::Slint)
