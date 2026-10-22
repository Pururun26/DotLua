#include "dotlua_save.h"
#include "raylib.h"

#include <stdio.h>
#include <string.h>
#include <direct.h>

#define SAVE_SLOTS  64
#define SAVE_BYTES  ((int)(SAVE_SLOTS * sizeof(int64_t)))   /* 512 */

static int64_t g_save[SAVE_SLOTS];
static char    g_save_path[2048];
static int     g_save_ready = 0;

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

    char base[256];
    cart_basename(cart_path, base, sizeof(base));

    char dir[1024];
    snprintf(dir, sizeof(dir), "%ssave", GetApplicationDirectory());
    _mkdir(dir);

    snprintf(g_save_path, sizeof(g_save_path), "%s/%s.bin", dir, base);

    FILE *f = fopen(g_save_path, "rb");
    if (f) {
        size_t r = fread(g_save, 1, sizeof(g_save), f);
        (void)r;
        fclose(f);
    }
    g_save_ready = 1;
}

void dotlua_save_close(void) {
    g_save_ready = 0;
}

void dotlua_dset(int slot, int64_t value) {
    if (!g_save_ready || slot < 0 || slot >= SAVE_SLOTS) return;
    if (g_save[slot] == value) return;

    g_save[slot] = value;

    FILE *f = fopen(g_save_path, "wb");
    if (!f) return;
    fwrite(g_save, 1, sizeof(g_save), f);
    fclose(f);
}

int64_t dotlua_dget(int slot) {
    if (slot < 0 || slot >= SAVE_SLOTS) return 0;
    return g_save[slot];
}