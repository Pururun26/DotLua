cmake_minimum_required(VERSION 3.15)
project(dotlua_libretro C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ============================================================
# Оптимизация под размер (Release)
# -ffunction-sections/-fdata-sections нужны, чтобы --gc-sections
# на линковке реально выкидывал неиспользуемые функции Lua/нашего кода.
# ============================================================
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS_RELEASE
        "-Os -DNDEBUG -ffunction-sections -fdata-sections \
         -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-ident")
endif()

# ============================================================
# Lua 5.4.9 — безопасный subset (без io/os/package/debug/linit)
# ============================================================
set(LUA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../lua-5.4.9")

set(LUA_SRC
    ${LUA_DIR}/lapi.c
    ${LUA_DIR}/lauxlib.c
    ${LUA_DIR}/lbaselib.c
    ${LUA_DIR}/lcode.c
    ${LUA_DIR}/lcorolib.c
    ${LUA_DIR}/lctype.c
    ${LUA_DIR}/ldebug.c
    ${LUA_DIR}/ldo.c
    ${LUA_DIR}/ldump.c
    ${LUA_DIR}/lfunc.c
    ${LUA_DIR}/lgc.c
    ${LUA_DIR}/llex.c
    ${LUA_DIR}/lmathlib.c
    ${LUA_DIR}/lmem.c
    ${LUA_DIR}/lobject.c
    ${LUA_DIR}/lopcodes.c
    ${LUA_DIR}/lparser.c
    ${LUA_DIR}/lstate.c
    ${LUA_DIR}/lstring.c
    ${LUA_DIR}/lstrlib.c
    ${LUA_DIR}/ltable.c
    ${LUA_DIR}/ltablib.c
    ${LUA_DIR}/ltm.c
    ${LUA_DIR}/lundump.c
    ${LUA_DIR}/lvm.c
    ${LUA_DIR}/lzio.c
)

add_library(dotlua_libretro SHARED
    src/dotlua_libretro.c
    src/dotlua_api.c
    src/dotlua_api_lua.c
    src/palette.c
    src/video.c
    ${LUA_SRC}
)

target_include_directories(dotlua_libretro PRIVATE
    libretro-common/include
    src
    ${LUA_DIR}
)

set_target_properties(dotlua_libretro PROPERTIES PREFIX "")

if(WIN32)
    set_target_properties(dotlua_libretro PROPERTIES
        SUFFIX ".dll"
        LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/dotlua_libretro.def")
    target_link_options(dotlua_libretro PRIVATE
        -static -static-libgcc -static-libstdc++
        -Wl,--gc-sections
        -s)
    target_link_libraries(dotlua_libretro PRIVATE -lpthread)
    target_link_libraries(dotlua_libretro PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/dotlua_libretro.def")
elseif(APPLE)
    set_target_properties(dotlua_libretro PROPERTIES SUFFIX ".dylib")
    target_link_options(dotlua_libretro PRIVATE
        -Wl,-dead_strip
        -Wl,-x
        -Wl,-S)
else()
    set_target_properties(dotlua_libretro PROPERTIES SUFFIX ".so")
    target_link_options(dotlua_libretro PRIVATE
        -Wl,--gc-sections
        -Wl,--as-needed
        -Wl,-O1
        -Wl,--hash-style=gnu
        -s)
endif()