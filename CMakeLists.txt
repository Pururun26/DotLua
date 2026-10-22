cmake_minimum_required(VERSION 3.15)
project(dotlua C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ============================================================
# Оптимизация под размер (вместо дефолтного -O3)
# Должно быть ДО add_executable/add_library — иначе не подхватится.
# ============================================================
if(EMSCRIPTEN)
    set(CMAKE_C_FLAGS_RELEASE "-Oz -DNDEBUG")
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG")
endif()

# ============================================================
# raylib — по платформе
# ============================================================
if(EMSCRIPTEN)
    set(RAYLIB_DIR      "${CMAKE_SOURCE_DIR}/raylib/raylib-6.0_webassembly")
    set(RAYLIB_LIB_NAME "raylib.web")
elseif(WIN32)
    set(RAYLIB_DIR      "${CMAKE_SOURCE_DIR}/raylib/raylib-6.0_win64_mingw-w64")
    set(RAYLIB_LIB_NAME "raylib")
elseif(APPLE)
    set(RAYLIB_DIR      "${CMAKE_SOURCE_DIR}/raylib/raylib-6.0_macos")
    set(RAYLIB_LIB_NAME "raylib")
else()
    set(RAYLIB_DIR      "${CMAKE_SOURCE_DIR}/raylib/raylib-6.0_linux_amd64")
    set(RAYLIB_LIB_NAME "raylib")
endif()

# ============================================================
# Lua 5.4.9 — безопасный subset (без io/os/package/debug/linit)
# ============================================================
set(LUA_SRC
    lua-5.4.9/lapi.c
    lua-5.4.9/lauxlib.c
    lua-5.4.9/lbaselib.c
    lua-5.4.9/lcode.c
    lua-5.4.9/lcorolib.c
    lua-5.4.9/lctype.c
    lua-5.4.9/ldebug.c
    lua-5.4.9/ldo.c
    lua-5.4.9/ldump.c
    lua-5.4.9/lfunc.c
    lua-5.4.9/lgc.c
    lua-5.4.9/llex.c
    lua-5.4.9/lmathlib.c
    lua-5.4.9/lmem.c
    lua-5.4.9/lobject.c
    lua-5.4.9/lopcodes.c
    lua-5.4.9/lparser.c
    lua-5.4.9/lstate.c
    lua-5.4.9/lstring.c
    lua-5.4.9/lstrlib.c
    lua-5.4.9/ltable.c
    lua-5.4.9/ltablib.c
    lua-5.4.9/ltm.c
    lua-5.4.9/lundump.c
    lua-5.4.9/lutf8lib.c
    lua-5.4.9/lvm.c
    lua-5.4.9/lzio.c
)

add_library(lua STATIC ${LUA_SRC})
target_include_directories(lua PUBLIC "${CMAKE_SOURCE_DIR}/lua-5.4.9")

# ============================================================
# DotLua
# ============================================================
add_executable(dotlua
    src/dotlua.c
    src/dotlua_api.c
    src/dotlua_api_lua.c
    src/dotlua_log.c
    src/dotlua_save.c
)

target_include_directories(dotlua PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${RAYLIB_DIR}/include"
)
target_link_directories(dotlua PRIVATE "${RAYLIB_DIR}/lib")

# ============================================================
# Web
# ============================================================
if(EMSCRIPTEN)
    target_link_libraries(dotlua PRIVATE ${RAYLIB_LIB_NAME} lua)
    target_link_options(dotlua PRIVATE
        -sUSE_GLFW=3
        -sASYNCIFY
        -sALLOW_MEMORY_GROWTH=1
        -sSTACK_SIZE=1MB
        "--shell-file=${CMAKE_SOURCE_DIR}/web/shell.html"
        "-sEXPORTED_RUNTIME_METHODS=['callMain','FS']"
        -Oz
    )
    set_target_properties(dotlua PROPERTIES
        SUFFIX      ".html"
        OUTPUT_NAME "index"
    )
    message(STATUS "Target: web (index.html)")

# ============================================================
# Windows / Linux / macOS
# ============================================================
else()

    # Выкинуть неиспользуемые функции/данные (требует -ffunction-sections -fdata-sections)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(dotlua PRIVATE
            -ffunction-sections
            -fdata-sections
        )
    endif()

    if(WIN32)
        target_link_libraries(dotlua PRIVATE ${RAYLIB_LIB_NAME} lua
            opengl32 gdi32 winmm shell32)
        # Один exe без DLL:
        target_link_options(dotlua PRIVATE
            -static -static-libgcc -static-libstdc++
            -lpthread
            -mwindows
            -Wl,--gc-sections
            -s
        )
    elseif(APPLE)
        find_library(COCOA_LIBRARY Cocoa REQUIRED)
        find_library(IOKIT_LIBRARY IOKit REQUIRED)
        find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
        find_library(OPENGL_LIBRARY OpenGL REQUIRED)
        target_link_libraries(dotlua PRIVATE ${RAYLIB_LIB_NAME} lua
            ${COCOA_LIBRARY} ${IOKIT_LIBRARY}
            ${COREVIDEO_LIBRARY} ${OPENGL_LIBRARY})
        target_link_options(dotlua PRIVATE -Wl,-dead_strip -s)
    else()
        find_package(Threads REQUIRED)
        target_link_libraries(dotlua PRIVATE ${RAYLIB_LIB_NAME} lua
            m ${CMAKE_DL_LIBS} Threads::Threads)
        target_link_options(dotlua PRIVATE -Wl,--gc-sections -s)
    endif()

    message(STATUS "Target: native (dotlua${CMAKE_EXECUTABLE_SUFFIX})")
endif()