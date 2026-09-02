# overlay triplet: identical to the community x64-osx triplet except that the
# skia port is built as a dynamic library, matching the arm64-osx overlay
# triplet so both macOS architectures ship the same way.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES x86_64)

if(PORT STREQUAL "skia")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()
