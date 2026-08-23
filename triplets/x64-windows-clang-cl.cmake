set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
# forward the compiler selection (cc/cxx=clang-cl) into dependency builds and the abi hash
set(VCPKG_ENV_PASSTHROUGH PATH CC CXX)
# ports add msvc-only flags (e.g. /MP) that clang-cl rejects as "argument unused"; silence those instead of patching every port
set(VCPKG_C_FLAGS "-Qunused-arguments")
set(VCPKG_CXX_FLAGS "-Qunused-arguments")
# vcpkg's windows toolchain hardcodes /c65001 into cmake_rc_flags, which cmake feeds to clang-cl when scanning .rc files; drop it (plain cache set, so a pre-set value wins)
set(VCPKG_CMAKE_CONFIGURE_OPTIONS "-DCMAKE_RC_FLAGS=/DWIN32")
