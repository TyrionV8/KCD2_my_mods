project(InstantPrepare LANGUAGES CXX)

find_package(minhook CONFIG REQUIRED)

add_library(InstantPrepare SHARED ../src/plugin.cpp)
target_precompile_headers(InstantPrepare PRIVATE "${RE_BUILDENV}/PCH.h")
target_compile_definitions(InstantPrepare PRIVATE
    _ITERATOR_DEBUG_LEVEL=0 WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(InstantPrepare PRIVATE /utf-8 /W4 /wd4819 /wd4100)
target_include_directories(InstantPrepare PRIVATE
    ${RE_ROOT}/include
    ${RE_ROOT}/include/CryEngine/CryCommon)
target_link_libraries(InstantPrepare PRIVATE kcd_re minhook::minhook)
