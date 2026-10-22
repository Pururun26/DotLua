#include "dotlua_conf.h"
#include "dotlua_api.h"
#include "dotlua_api_lua.h"
#include "dotlua_log.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "raylib.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>   /* va_list для dotlua_err */

/* Дублирует сообщение в stderr (сразу видно) и в dotlua_log.txt (остаётся на диске). */
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
    luaL_requiref(L, LUA_UTF8LIBNAME,  luaopen_utf8,      1); lua_pop(L, 1);

    /* base открыт — но loadfile/dofile читают файлы с диска,
       обходя лимит DOTLUA_MAX_SCRIPT_SIZE. Убираем.
       load оставляем — компилирует строку из памяти, не файл. */
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

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <cartridge.dlua>\n", argv[0]);
        return 1;
    }
    if (!has_dlua_ext(argv[1])) {
        fprintf(stderr, "dotlua: expected .dlua cartridge, got: %s\n", argv[1]);
        return 1;
    }

    dotlua_log_init();
    dotlua_log("starting: %s", argv[1]);

    /* --- проверка размера картриджа --- */
    long sz = file_size(argv[1]);
    if (sz < 0) {
        dotlua_err("dotlua: cannot read '%s'\n", argv[1]);
        dotlua_log_close();
        return 1;
    }
    dotlua_log("[INFO] DotLua: Script size %ld / %d chars", sz, DOTLUA_MAX_SCRIPT_SIZE);
    if (sz > DOTLUA_MAX_SCRIPT_SIZE) {
        dotlua_err("dotlua: script too large: %ld / %d\n", sz, DOTLUA_MAX_SCRIPT_SIZE);
        dotlua_log_close();
        return 1;
    }

    lua_State *L = luaL_newstate();
    if (!L) {
        dotlua_err("dotlua: failed to create lua state\n");
        dotlua_log_close();
        return 1;
    }

    dotlua_openlibs(L);
    dotlua_register_api(L);

    if (luaL_loadfile(L, argv[1]) != LUA_OK) {
        dotlua_err("dotlua: load error: %s\n", lua_tostring(L, -1));
        lua_close(L);
        dotlua_log_close();
        return 1;
    }
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        dotlua_err("dotlua: runtime error: %s\n", lua_tostring(L, -1));
        lua_close(L);
        dotlua_log_close();
        return 1;
    }

    InitWindow(DOTLUA_WINDOW_WIDTH, DOTLUA_WINDOW_HEIGHT, DOTLUA_TITLE);
    InitAudioDevice();
    HideCursor();
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(DOTLUA_FPS);

    dotlua_api_init();

    int show_fps = 0;                    /* FPS-оверлей выключен по умолчанию */

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        if (IsKeyPressed(KEY_P) && IsKeyDown(KEY_LEFT_CONTROL))
            show_fps = !show_fps;        /* Ctrl+P переключает */

        dotlua_poll_buttons();

        if (call_update(L)) break;

        if (show_fps) {                  /* оверлей: после update, до present */
            char buf[16];
            snprintf(buf, sizeof(buf), "FPS:%d", dotlua_fps_get());
            int len = (int)strlen(buf);
            int w   = len * 4;           /* 3px глиф + 1px gap, верхняя граница */
            dotlua_print(buf, DOTLUA_WIDTH - w + 1, 1, 1);
        }

        BeginDrawing();
            ClearBackground(BLACK);
            dotlua_present();
        EndDrawing();

        dotlua_fps_frame();              /* счётчик: один раз на кадр */
    }

    dotlua_api_shutdown();
    CloseAudioDevice();
    CloseWindow();
    lua_close(L);
    dotlua_log_close();
    return 0;
}