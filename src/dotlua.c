#include "dotlua_conf.h"
#include "dotlua_api.h"
#include "dotlua_api_lua.h"
#include "dotlua_log.h"
#include "dotlua_save.h"
#include "dotlua_console.h"
#include "dotlua_license.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "raylib.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* Дублирует сообщение в stderr и в dotlua_log.txt. */
static void dotlua_err(const char *fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    fprintf(stderr, "%s", buf);
    dotlua_log("%s", buf);
}

static void dotlua_openlibs(lua_State *L) {
    luaL_requiref(L, LUA_GNAME,        luaopen_base,      1); lua_pop(L, 1);
    luaL_requiref(L, LUA_MATHLIBNAME,  luaopen_math,      1); lua_pop(L, 1);
    luaL_requiref(L, LUA_STRLIBNAME,   luaopen_string,    1); lua_pop(L, 1);
    luaL_requiref(L, LUA_TABLIBNAME,   luaopen_table,     1); lua_pop(L, 1);

    lua_pushnil(L); lua_setglobal(L, "loadfile");
    lua_pushnil(L); lua_setglobal(L, "dofile");
}

static int call_update(lua_State *L) {
    lua_getglobal(L, "update");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        dotlua_err("dotlua: update error: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        return 1;
    }
    return 0;
}

static long file_size(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    return sz;
}

static int has_dlua_ext(const char *path) {
    size_t n = strlen(path);
    return n >= 5 && strcmp(path + n - 5, ".dlua") == 0;
}

/* --- Состояние для главного цикла (нужно emscripten_set_main_loop) --- */
static lua_State *g_L        = NULL;
static int        g_show_fps = 0;
static int        g_running  = 1;

static void frame(void) {
    if (WindowShouldClose()) {
        g_running = 0;
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        return;
    }

#ifndef __EMSCRIPTEN__
    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();
#endif

    if (IsKeyPressed(KEY_P) && IsKeyDown(KEY_LEFT_CONTROL))
        g_show_fps = !g_show_fps;

    dotlua_poll_buttons();

    if (call_update(g_L)) {
        g_running = 0;
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        return;
    }

    if (g_show_fps) {
        char buf[16];
        snprintf(buf, sizeof(buf), "FPS:%d", dotlua_fps_get());
        int len = (int)strlen(buf);
        int w   = len * 4;
        dotlua_print(buf, DOTLUA_WIDTH - w + 1, 1, 1);
    }

    BeginDrawing();
        ClearBackground(BLACK);
        dotlua_present();
    EndDrawing();

    dotlua_fps_frame();
}

int main(int argc, char **argv) {
    // CLI-флаги (до InitWindow, до Lua, до поиска картриджа)
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--license") == 0) {
            dotlua_console_attach();
            printf("\n%s", DOTLUA_LICENSE_TEXT);
            fflush(stdout);
            dotlua_console_detach();
            return 0;
        }

        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            dotlua_console_attach();
            printf("\nDotLua " DOTLUA_VERSION " - a tiny fantasy console for Lua\n\n");
            printf("Usage:\n");
            printf("  dotlua.exe                  run cart.dlua next to the exe\n");
            printf("  dotlua.exe mygame.dlua      run a specific cartridge\n");
            printf("  dotlua.exe --license        show licenses\n");
            printf("  dotlua.exe --help           show this help\n");
            printf("  dotlua.exe --version        show version\n");
            fflush(stdout);
            dotlua_console_detach();
            return 0;
        }

        if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            dotlua_console_attach();
            printf("DotLua v%s\n", DOTLUA_VERSION);
            fflush(stdout);
            dotlua_console_detach();
            return 0;
        }
    }

    const char *cart = NULL;
    char cart_buf[1024];

    if (argc >= 2) {
        cart = argv[1];
    } else {
        /* Нет аргумента — ищем cart.dlua рядом с exe */
        snprintf(cart_buf, sizeof(cart_buf), "%scart.dlua", GetApplicationDirectory());
        if (file_size(cart_buf) < 0) {
            fprintf(stderr, "usage: %s <cartridge.dlua>\n", argv[0]);
            return 1;
        }
        cart = cart_buf;
    }

    if (!has_dlua_ext(cart)) {
        fprintf(stderr, "dotlua: expected .dlua cartridge, got: %s\n", cart);
        return 1;
    }

    dotlua_log_init();
    dotlua_log("starting: %s", cart);

    long sz = file_size(cart);
    if (sz < 0) {
        dotlua_err("dotlua: cannot read '%s'\n", cart);
        dotlua_log_close();
        return 1;
    }
    dotlua_log("[INFO] DotLua: Script size %ld / %d chars", sz, DOTLUA_MAX_SCRIPT_SIZE);
    if (sz > DOTLUA_MAX_SCRIPT_SIZE) {
        dotlua_err("dotlua: script too large: %ld / %d\n", sz, DOTLUA_MAX_SCRIPT_SIZE);
        dotlua_log_close();
        return 1;
    }

    dotlua_save_open(cart);

    g_L = luaL_newstate();
    if (!g_L) {
        dotlua_err("dotlua: failed to create lua state\n");
        dotlua_log_close();
        return 1;
    }

    dotlua_openlibs(g_L);
    dotlua_register_api(g_L);

    if (luaL_loadfile(g_L, cart) != LUA_OK) {
        dotlua_err("dotlua: load error: %s\n", lua_tostring(g_L, -1));
        lua_close(g_L);
        dotlua_log_close();
        return 1;
    }
    if (lua_pcall(g_L, 0, 0, 0) != LUA_OK) {
        dotlua_err("dotlua: runtime error: %s\n", lua_tostring(g_L, -1));
        lua_close(g_L);
        dotlua_log_close();
        return 1;
    }

    InitWindow(DOTLUA_WINDOW_WIDTH, DOTLUA_WINDOW_HEIGHT, DOTLUA_TITLE);
    InitAudioDevice();
    HideCursor();

#ifndef __EMSCRIPTEN__
    SetWindowState(FLAG_WINDOW_RESIZABLE);
#endif
    SetTargetFPS(DOTLUA_FPS);

    dotlua_api_init();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame, 0, 1);
#else
    while (g_running) frame();

    dotlua_api_shutdown();
    CloseAudioDevice();
    CloseWindow();
    dotlua_save_close();
    lua_close(g_L);
    dotlua_log_close();
#endif
    return 0;
}