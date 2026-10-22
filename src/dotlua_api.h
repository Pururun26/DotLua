#pragma once

#include "dotlua_conf.h"
#include "raylib.h"

extern unsigned char g_fb[DOTLUA_WIDTH * DOTLUA_HEIGHT];
extern Color         g_palette[DOTLUA_COLOR_COUNT];

/* InitWindow() -> dotlua_api_init() -> ... -> dotlua_api_shutdown() -> CloseWindow() */
void dotlua_api_init(void);
void dotlua_api_shutdown(void);

/* Вызывать между BeginDrawing() и EndDrawing(). */
void dotlua_present(void);

void dotlua_fps_frame(void);
int  dotlua_fps_get(void);

void dotlua_cls (int color);
void dotlua_pset(int x, int y, int color);
int  dotlua_pget(int x, int y);

void dotlua_line(int x0, int y0, int x1, int y1, int color);
void dotlua_rect(int x, int y, int w, int h, int color);
void dotlua_oval(int x, int y, int w, int h, int color);
void dotlua_print(const char *s, int x, int y, int color);

void dotlua_sprdata(int id, const char *art);
void dotlua_spr    (int id, int x, int y);

long long dotlua_utime(void);

void dotlua_poll_buttons(void);
int  dotlua_btn (int i);
int  dotlua_btnp(int i);

void dotlua_beep(int freq, int dur, int vol);