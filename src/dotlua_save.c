#include "dotlua_save.h"
#include "raylib.h"

#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EM_JS(int, dotlua_ls_load, (void *ptr, int size), {
    try {
        var b64 = localStorage.getItem('storage');
        if (!b64) return 0;

        var bin = atob(b64);
        if (bin.length !== size) return 0;

        var bytes = new Uint8Array(bin.length);
        for (var i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i);
        HEAPU8.set(bytes, ptr);
        return 1;
    } catch (e) {
        console.error('[DotLua] localStorage read failed:', e);
        return 0;
    }
});

EM_JS(int, dotlua_ls_save, (void *ptr, int size), {
    try {
        var bytes = HEAPU8.subarray(ptr, ptr + size);
        var bin = "";
        for (var i = 0; i < bytes.length; i++) bin += String.fromCharCode(bytes[i]);
        localStorage.setItem('storage', btoa(bin));
        return 1;
    } catch (e) {
        console.error('[DotLua] localStorage write failed:', e);
        return 0;
    }
});
#endif

#define SAVE_SLOTS  64

static int64_t g_save[SAVE_SLOTS];
static int     g_save_ready = 0;

#ifndef __EMSCRIPTEN__
static char g_save_path[2048];
#endif

static void cart_basename(const char *cart_path, char *out, size_t cap) {
    const char *s  = strrchr(cart_path, '/');
    const char *bs = strrchr(cart_path, '\\');
    if (!s || (bs && bs > s)) s = bs;
    s = s ? s + 1 : cart_path;

    const char *dot = strrchr(s, '.');
    size_t n = dot ? (size_t)(dot - s) : strlen(s);
    if (n >= cap) n = cap - 1;
    memcpy(out, s, n);
    out[n] = 0;
}

void dotlua_save_open(const char *cart_path) {
    memset(g_save, 0, sizeof(g_save));
    g_save_ready = 0;

#ifdef __EMSCRIPTEN__
    /* Web: один ключ 'storage' — wasm = одна игра */
    int loaded = dotlua_ls_load(g_save, (int)sizeof(g_save));
    TraceLog(LOG_INFO, "DotLua: save %s", loaded ? "loaded" : "fresh");
#else
    char base[256];
    cart_basename(cart_path, base, sizeof(base));

    char dir[1024];
    snprintf(dir, sizeof(dir), "%ssave", GetApplicationDirectory());
    if (!DirectoryExists(dir)) MakeDirectory(dir);

    snprintf(g_save_path, sizeof(g_save_path), "%s/%s.bin", dir, base);

    if (FileExists(g_save_path)) {
        int size = 0;
        unsigned char *data = LoadFileData(g_save_path, &size);
        if (data) {
            if (size == (int)sizeof(g_save)) {
                memcpy(g_save, data, sizeof(g_save));
                TraceLog(LOG_INFO, "DotLua: save loaded '%s'", g_save_path);
            } else {
                TraceLog(LOG_WARNING, "DotLua: save size %d != %d", size, (int)sizeof(g_save));
            }
            UnloadFileData(data);
        }
    } else {
        TraceLog(LOG_INFO, "DotLua: save fresh '%s'", g_save_path);
    }
#endif
    g_save_ready = 1;
}

void dotlua_save_close(void) {
    g_save_ready = 0;
}

void dotlua_dset(int slot, int64_t value) {
    if (!g_save_ready || slot < 0 || slot >= SAVE_SLOTS) return;
    if (g_save[slot] == value) return;

    g_save[slot] = value;

#ifdef __EMSCRIPTEN__
    dotlua_ls_save(g_save, (int)sizeof(g_save));
#else
    SaveFileData(g_save_path, g_save, sizeof(g_save));
#endif
}

int64_t dotlua_dget(int slot) {
    if (slot < 0 || slot >= SAVE_SLOTS) return 0;
    return g_save[slot];
}