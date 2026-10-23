#pragma once
#include <stdint.h>
#include <stddef.h>

#define DOTLUA_W 96
#define DOTLUA_H 96

/* Прямой доступ к 0/1 framebuffer (DOTLUA_W × DOTLUA_H байт, row-major).
 * Caller отвечает за bounds-check. Используется для быстрых примитивов. */
unsigned char *dotlua_video_fb(void);

void        dotlua_video_init(void);
void        dotlua_video_deinit(void);

/* Залить весь fb цветом (0 или 1). */
void        dotlua_video_clear(int color);

/* Поставить пиксель (0 или 1), клипается по границам. */
void        dotlua_video_pset(int x, int y, int color);

/* Прочитать пиксель, 0 или 1. */
int         dotlua_video_pget(int x, int y);

/* Готовый буфер для video_cb() — XRGB8888, width*height*4 байт. */
const void *dotlua_video_get_framebuffer(void);
unsigned    dotlua_video_get_width(void);
unsigned    dotlua_video_get_height(void);
size_t      dotlua_video_get_pitch(void);   /* width * 4 */