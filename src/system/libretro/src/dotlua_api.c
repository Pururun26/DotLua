#include "dotlua_api.h"
#include "dotlua_font3x4.h"
#include "video.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

void dotlua_cls(int color) {
    dotlua_video_clear(color);
}

void dotlua_pset(int x, int y, int color) {
    dotlua_video_pset(x, y, color);
}

int dotlua_pget(int x, int y) {
    return dotlua_video_pget(x, y);
}

/* ---------- print ---------- */
void dotlua_print(const char *s, int x, int y, int color) {
    if (!s) return;
    int cx = x, cy = y;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        if (*p == '\n') { cx = x; cy += FONT_CELL_H + 1; continue; }
        if (*p == '\r') continue;

        unsigned char u = *p;
        if (u >= 'a' && u <= 'z') u -= 32;

        int idx = (int)u - FONT_FIRST;
        if (idx < 0 || idx >= FONT_COUNT) idx = '?' - FONT_FIRST;

        int w = dotlua_font_w[idx];
        for (int row = 0; row < FONT_CELL_H; row++) {
            unsigned char bits = dotlua_font[idx][row];
            for (int col = 0; col < FONT_CELL_W; col++) {
                if ((bits >> (FONT_CELL_W - 1 - col)) & 1)
                    dotlua_pset(cx + col, cy + row, color);
            }
        }

        cx += w + 1;
    }
}


/* ---------- sprites ---------- */
/* 0 = цвет 0, 1 = цвет 1, DOTLUA_TRANSPARENT = не рисовать */
static unsigned char g_sprites[DOTLUA_NUM_SPRITES][DOTLUA_SPRITE_SIZE * DOTLUA_SPRITE_SIZE];

void dotlua_sprdata(int id, const char *art) {
    if (id < 0 || id >= DOTLUA_NUM_SPRITES || !art) return;

    /* По умолчанию всё прозрачное — повторный sprdata не оставит хвоста */
    memset(g_sprites[id], DOTLUA_TRANSPARENT, sizeof(g_sprites[id]));

    int row = 0, col = 0;
    for (const unsigned char *p = (const unsigned char *)art; *p; ) {
        if (*p == '\n') {
            if (col > 0) { row++; col = 0; }   /* пропускаем ведущий пустой \n */
            p++;
            continue;
        }
        if (*p == '\r') { p++; continue; }

        unsigned char v;
        if      (p[0] == '#' || p[0] == '1')               v = 1;
        else if (p[0] == '0')                              v = 0;
        else if (p[0] == '3' || p[0] == '.' || p[0] == ' ') v = DOTLUA_TRANSPARENT;
        else { p++; continue; }                            /* неизвестный — пропуск */

        if (row < DOTLUA_SPRITE_SIZE && col < DOTLUA_SPRITE_SIZE)
            g_sprites[id][row * DOTLUA_SPRITE_SIZE + col] = v;

        col++;
        p++;
    }
}

void dotlua_spr(int id, int x, int y) {
    if (id < 0 || id >= DOTLUA_NUM_SPRITES) return;

    const unsigned char *sp = g_sprites[id];
    for (int r = 0; r < DOTLUA_SPRITE_SIZE; r++) {
        for (int c = 0; c < DOTLUA_SPRITE_SIZE; c++) {
            unsigned char v = sp[r * DOTLUA_SPRITE_SIZE + c];
            if (v == DOTLUA_TRANSPARENT) continue;
            dotlua_pset(x + c, y + r, v);   /* bounds-check внутри dotlua_pset */
        }
    }
}


/* ---------- input ---------- */
static int g_btn_curr = 0;
static int g_btn_prev = 0;

void dotlua_btn_update(int mask) {
    g_btn_prev = g_btn_curr;
    g_btn_curr = mask;
}

int dotlua_btn(int i) {
    if (i < 0 || i >= 6) return 0;
    return (g_btn_curr >> i) & 1;
}

int dotlua_btnp(int i) {
    if (i < 0 || i >= 6) return 0;
    int edge = g_btn_curr & ~g_btn_prev;
    return (edge >> i) & 1;
}

/* ---------- time (ms since start) ---------- */

static int64_t dotlua_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + (int64_t)ts.tv_nsec / 1000000;
}

static int64_t g_start_ms = 0;

void dotlua_time_reset(void) {
    g_start_ms = dotlua_now_ms();
}

int64_t dotlua_time(void) {
    return dotlua_now_ms() - g_start_ms;
}

/* ---------- utime ---------- */
int64_t dotlua_utime(void) {
    return (int64_t)time(NULL);
}


/* ---------- save ---------- */
static int64_t g_save[DOTLUA_SAVE_SLOTS];

void dotlua_dset(int slot, int64_t value) {
    if (slot < 0 || slot >= DOTLUA_SAVE_SLOTS) return;
    g_save[slot] = value;
}

int64_t dotlua_dget(int slot) {
    if (slot < 0 || slot >= DOTLUA_SAVE_SLOTS) return 0;
    return g_save[slot];
}

void  *dotlua_save_data(void) { return g_save; }
size_t dotlua_save_size(void) { return sizeof(g_save); }


/* ---------- beep ---------- */
typedef struct {
    int     active;
    float   phase;        /* 0..1 */
    int64_t remaining;    /* сэмплов до конца */
    int     freq;         /* 0 = noise */
    int     vol;          /* 0..100 */
} Tone;

static Tone g_tone = {0};

void dotlua_beep(int freq, int dur, int vol) {
    if (dur <= 0 || vol <= 0) {
        g_tone.active = 0;
        g_tone.remaining = 0;
        return;
    }
    if (freq < 0) freq = 0;
    if (freq > 8000) freq = 8000;
    if (dur  > 255) dur  = 255;
    if (vol  > 100) vol  = 100;

    float dur_sec = (float)dur / (float)DOTLUA_FPS;
    int64_t samples = (int64_t)(DOTLUA_SAMPLE_RATE * dur_sec);
    if (samples <= 0) return;

    g_tone.active    = 1;
    g_tone.phase     = 0.0f;
    g_tone.remaining = samples;
    g_tone.freq      = freq;
    g_tone.vol       = vol;
}

void dotlua_audio_render(int16_t *out, int frames) {
    for (int i = 0; i < frames; i++) {
        int16_t s = 0;

        if (g_tone.active && g_tone.remaining > 0) {
            float v;
            if (g_tone.freq == 0) {
                v = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;   /* noise */
            } else {
                v = (g_tone.phase < 0.5f) ? 1.0f : -1.0f;              /* square */
                g_tone.phase += (float)g_tone.freq / (float)DOTLUA_SAMPLE_RATE;
                if (g_tone.phase >= 1.0f) g_tone.phase -= 1.0f;
            }
            s = (int16_t)(v * g_tone.vol * 327.67f);
            g_tone.remaining--;
        }

        out[i * 2]     = s;   /* L */
        out[i * 2 + 1] = s;   /* R */
    }
}


/* ---------- line (Bresenham) ---------- */
void dotlua_line(int x0, int y0, int x1, int y1, int color) {
    int dx =  abs(x1 - x0);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        dotlua_pset(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void dotlua_rect(int x, int y, int w, int h, int color) {
    if (w <= 0 || h <= 0) return;

    int x0 = x,        y0 = y;
    int x1 = x + w,    y1 = y + h;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > DOTLUA_W) x1 = DOTLUA_W;
    if (y1 > DOTLUA_H) y1 = DOTLUA_H;
    if (x0 >= x1 || y0 >= y1) return;

    unsigned char cc = (unsigned char)(color & 1);
    unsigned char *fb = dotlua_video_fb();
    for (int yy = y0; yy < y1; yy++) {
        unsigned char *row = &fb[yy * DOTLUA_W];
        for (int xx = x0; xx < x1; xx++) row[xx] = cc;
    }
}

/* ---------- oval (залитый, круг = w==h) ---------- */
void dotlua_oval(int x, int y, int w, int h, int color) {
    if (w <= 0 || h <= 0) return;

    double cx = x + w / 2.0;
    double cy = y + h / 2.0;
    double rx = w / 2.0;
    double ry = h / 2.0;
    if (rx < 0.5) rx = 0.5;
    if (ry < 0.5) ry = 0.5;

    int y0 = y, y1 = y + h;
    if (y0 < 0) y0 = 0;
    if (y1 > DOTLUA_H) y1 = DOTLUA_H;

    unsigned char cc = (unsigned char)(color & 1);
    unsigned char *fb = dotlua_video_fb();
    for (int py = y0; py < y1; py++) {
        double dy = (py + 0.5 - cy) / ry;
        if (dy < -1.0 || dy > 1.0) continue;
        double dx = sqrt(1.0 - dy * dy) * rx;

        int xs = (int)(cx - dx + 0.5);
        int xe = (int)(cx + dx + 0.5);
        if (xs < 0) xs = 0;
        if (xe > DOTLUA_W) xe = DOTLUA_W;
        if (xs >= xe) continue;

        unsigned char *row = &fb[py * DOTLUA_W];
        for (int px = xs; px < xe; px++) row[px] = cc;
    }
}