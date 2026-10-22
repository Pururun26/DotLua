#include "dotlua_api.h"
#include "dotlua_font3x4.h"

#include <stdlib.h>   /* abs */
#include <math.h>     /* sqrt */
#include <string.h>   /* memset */
#include <time.h>

unsigned char g_fb[DOTLUA_WIDTH * DOTLUA_HEIGHT];
Color         g_palette[DOTLUA_COLOR_COUNT];

static Color   g_pixels[DOTLUA_WIDTH * DOTLUA_HEIGHT];
static Texture2D g_tex;

void dotlua_api_init(void) {
    g_palette[0] = GetColor(DOTLUA_DEFAULT_COLOR_0);
    g_palette[1] = GetColor(DOTLUA_DEFAULT_COLOR_1);

    Image img = {
        .data    = g_pixels,
        .width   = DOTLUA_WIDTH,
        .height  = DOTLUA_HEIGHT,
        .mipmaps = 1,
        .format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };
    g_tex = LoadTextureFromImage(img);
    SetTextureFilter(g_tex, TEXTURE_FILTER_POINT);
}

void dotlua_api_shutdown(void) {
    UnloadTexture(g_tex);
}

void dotlua_present(void) {
    for (int i = 0; i < DOTLUA_WIDTH * DOTLUA_HEIGHT; i++)
        g_pixels[i] = g_palette[g_fb[i] & (DOTLUA_COLOR_COUNT - 1)];
    UpdateTexture(g_tex, g_pixels);

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int sx = sw / DOTLUA_WIDTH;
    int sy = sh / DOTLUA_HEIGHT;
    int s  = (sx < sy) ? sx : sy;
    if (s < 1) s = 1;

    int dw = DOTLUA_WIDTH  * s;
    int dh = DOTLUA_HEIGHT * s;
    int ox = (sw - dw) / 2;
    int oy = (sh - dh) / 2;

    DrawTexturePro(g_tex,
        (Rectangle){ 0, 0, (float)DOTLUA_WIDTH, (float)DOTLUA_HEIGHT },
        (Rectangle){ (float)ox, (float)oy, (float)dw, (float)dh },
        (Vector2){ 0, 0 }, 0.0f, WHITE);
}


static int g_fps_value  = 0;
static int g_fps_frames = 0;
static long long g_fps_last = 0;

void dotlua_fps_frame(void) {
    g_fps_frames++;
    long long now = dotlua_utime();
    if (g_fps_last == 0) g_fps_last = now;
    if (now - g_fps_last >= 1) {
        g_fps_value  = g_fps_frames;
        g_fps_frames = 0;
        g_fps_last   = now;
    }
}

int dotlua_fps_get(void) {
    return g_fps_value;
}

void dotlua_cls(int color) {
    unsigned char c = (unsigned char)(color & (DOTLUA_COLOR_COUNT - 1));
    for (int i = 0; i < DOTLUA_WIDTH * DOTLUA_HEIGHT; i++)
        g_fb[i] = c;
}

void dotlua_pset(int x, int y, int color) {
    if (x < 0 || x >= DOTLUA_WIDTH || y < 0 || y >= DOTLUA_HEIGHT) return;
    g_fb[y * DOTLUA_WIDTH + x] = (unsigned char)(color & (DOTLUA_COLOR_COUNT - 1));
}

int dotlua_pget(int x, int y) {
    if (x < 0 || x >= DOTLUA_WIDTH || y < 0 || y >= DOTLUA_HEIGHT) return 0;
    return (int)g_fb[y * DOTLUA_WIDTH + x];
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

/* ---------- rect (залитый) ---------- */
void dotlua_rect(int x, int y, int w, int h, int color) {
    if (w <= 0 || h <= 0) return;

    int x0 = x,        y0 = y;
    int x1 = x + w,    y1 = y + h;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > DOTLUA_WIDTH)  x1 = DOTLUA_WIDTH;
    if (y1 > DOTLUA_HEIGHT) y1 = DOTLUA_HEIGHT;
    if (x0 >= x1 || y0 >= y1) return;

    unsigned char cc = (unsigned char)(color & (DOTLUA_COLOR_COUNT - 1));
    for (int yy = y0; yy < y1; yy++) {
        unsigned char *row = &g_fb[yy * DOTLUA_WIDTH];
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
    if (y1 > DOTLUA_HEIGHT) y1 = DOTLUA_HEIGHT;

    unsigned char cc = (unsigned char)(color & (DOTLUA_COLOR_COUNT - 1));
    for (int py = y0; py < y1; py++) {
        double dy = (py + 0.5 - cy) / ry;
        if (dy < -1.0 || dy > 1.0) continue;
        double dx = sqrt(1.0 - dy * dy) * rx;

        int xs = (int)(cx - dx + 0.5);
        int xe = (int)(cx + dx + 0.5);
        if (xs < 0) xs = 0;
        if (xe > DOTLUA_WIDTH) xe = DOTLUA_WIDTH;
        if (xs >= xe) continue;

        unsigned char *row = &g_fb[py * DOTLUA_WIDTH];
        for (int px = xs; px < xe; px++) row[px] = cc;
    }
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
        if      (p[0] == '#' || p[0] == '1')              v = 1;
        else if (p[0] == '0')                             v = 0;
        else if (p[0] == '3' || p[0] == '.' || p[0] == ' ') v = DOTLUA_TRANSPARENT;
        else { p++; continue; }                           /* неизвестный — пропуск */

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


/* ---------- utime ---------- */
long long dotlua_utime(void) {
    return (long long)time(NULL);
}


/* ---------- buttons ---------- */
static int prev_btn = 0;
static int curr_btn = 0;

static int host_read_buttons(void) {
    int b = 0;
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) b |= 1 << 0;   /* LEFT  */
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) b |= 1 << 1;   /* RIGHT */
    if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) b |= 1 << 2;   /* UP    */
    if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) b |= 1 << 3;   /* DOWN  */
    if (IsKeyDown(KEY_Z)     || IsKeyDown(KEY_J)) b |= 1 << 4;   /* A     */
    if (IsKeyDown(KEY_X)     || IsKeyDown(KEY_K)) b |= 1 << 5;   /* B     */
    return b;
}

void dotlua_poll_buttons(void) {
    prev_btn = curr_btn;
    curr_btn = host_read_buttons();
}

int dotlua_btn(int i) {
    if (i < 0 || i >= 6) return 0;
    return (curr_btn & (1 << i)) ? 1 : 0;
}

int dotlua_btnp(int i) {
    if (i < 0 || i >= 6) return 0;
    int edge = curr_btn & ~prev_btn;
    return (edge & (1 << i)) ? 1 : 0;
}


/* ---------- beep ---------- */
#define BEEP_SAMPLE_RATE 44100

static Sound g_beep_sound;
static int   g_beep_used = 0;

static void beep_gen_square(short *s, int n, float freq, int vol) {
    for (int i = 0; i < n; i++) {
        float t     = (float)i / BEEP_SAMPLE_RATE;
        float phase = fmodf(t * freq, 1.0f);
        float v     = (phase < 0.5f) ? 1.0f : -1.0f;   /* 50% duty — квадрат */
        s[i] = (short)(v * vol * 327.67f);
    }
}

static void beep_stop(void) {
    if (g_beep_used) {
        StopSound(g_beep_sound);
        UnloadSound(g_beep_sound);
        g_beep_used = 0;
    }
}

void dotlua_beep(int freq, int dur, int vol) {
    if (!IsAudioDeviceReady()) return;

    if (freq <= 0 || dur <= 0 || vol <= 0) {
        beep_stop();
        return;
    }

    /* клампим, чтобы не словить переполнение/огромный malloc */
    if (freq >  8000) freq =  8000;
    if (dur  >   255) dur  =   255;
    if (vol  >   100) vol  =   100;

    float duration_sec = (float)dur / (float)DOTLUA_FPS;
    int   n = (int)(BEEP_SAMPLE_RATE * duration_sec);
    if (n <= 0) return;

    short *samples = (short *)malloc(n * sizeof(short));
    if (!samples) return;

    beep_gen_square(samples, n, (float)freq, vol);

    Wave wave = {
        .frameCount = (unsigned int)n,
        .sampleRate = BEEP_SAMPLE_RATE,
        .sampleSize = 16,
        .channels   = 1,
        .data       = samples,
    };
    Sound snd = LoadSoundFromWave(wave);
    free(samples);
    if (snd.frameCount == 0) return;

    beep_stop();      /* вытесняем предыдущий */

    g_beep_sound = snd;
    g_beep_used  = 1;
    PlaySound(snd);
}