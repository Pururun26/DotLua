#include "video.h"
#include "palette.h"
#include <string.h>

static uint8_t  g_fb[DOTLUA_W * DOTLUA_H];        /* 0/1 на пиксель */
static uint32_t g_pixels[DOTLUA_W * DOTLUA_H];    /* XRGB8888 */

void dotlua_video_init(void) {
    memset(g_fb, 0, sizeof(g_fb));
    memset(g_pixels, 0, sizeof(g_pixels));
}

void dotlua_video_deinit(void) {
    /* ничего */
}

void dotlua_video_clear(int color) {
    uint8_t c = (uint8_t)(color & 1);
    memset(g_fb, c, sizeof(g_fb));
}

void dotlua_video_pset(int x, int y, int color) {
    if (x < 0 || x >= DOTLUA_W || y < 0 || y >= DOTLUA_H) return;
    g_fb[y * DOTLUA_W + x] = (uint8_t)(color & 1);
}

int dotlua_video_pget(int x, int y) {
    if (x < 0 || x >= DOTLUA_W || y < 0 || y >= DOTLUA_H) return 0;
    return g_fb[y * DOTLUA_W + x];
}

const void *dotlua_video_get_framebuffer(void) {
    /* fb (0/1) → XRGB8888, перед video_cb */
    for (int i = 0; i < DOTLUA_W * DOTLUA_H; i++)
        g_pixels[i] = dotlua_palette[g_fb[i] & 1];
    return g_pixels;
}

unsigned dotlua_video_get_width(void)  { return DOTLUA_W; }
unsigned dotlua_video_get_height(void) { return DOTLUA_H; }
size_t   dotlua_video_get_pitch(void)  { return DOTLUA_W * sizeof(uint32_t); }

unsigned char *dotlua_video_fb(void) {
    return g_fb;
}