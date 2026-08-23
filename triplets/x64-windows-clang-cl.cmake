set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
# forward the compiler selection (cc/cxx=clang-cl) into dependency builds and the abi hash
set(VCPKG_ENV_PASSTHROUGH PATH CC CXX)
