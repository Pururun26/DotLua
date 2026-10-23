#pragma once
#include <stdint.h>
#include <stddef.h>

/* Save: 64 слота × int64 = 512 байт. */
#define DOTLUA_SAVE_SLOTS  64

/* Спрайты: 8x8, 256 слотов. */
#define DOTLUA_NUM_SPRITES  256
#define DOTLUA_SPRITE_SIZE  8
#define DOTLUA_TRANSPARENT   255

#define DOTLUA_FPS 30

/* Графика */
void dotlua_cls  (int color);
void dotlua_pset (int x, int y, int color);
int  dotlua_pget (int x, int y);
void dotlua_print(const char *s, int x, int y, int color);

/* Спрайты */
void dotlua_sprdata(int id, const char *art);
void dotlua_spr    (int id, int x, int y);

/* Ввод: 6 кнопок (0..5). */
void dotlua_btn_update(int mask);   /* 1 раз в кадр, до update() */
int  dotlua_btn (int i);
int  dotlua_btnp(int i);

/* Время */
int64_t dotlua_time (void);    /* ms since cartridge load */
int64_t dotlua_utime(void);    /* Unix timestamp, seconds */
void    dotlua_time_reset(void);

/* Сохранение */
void    dotlua_dset(int slot, int64_t value);
int64_t dotlua_dget(int slot);

/* Для libretro: указатель на 512-байтный массив (retro_get_memory_*) */
void   *dotlua_save_data(void);
size_t  dotlua_save_size(void);


/* Звук */
void dotlua_beep(int freq, int dur, int vol);

/* Заполнить буфер interleaved stereo int16 (L,R,L,R,...). */
void dotlua_audio_render(int16_t *out, int frames);

/* Сколько сэмплов в секунду — для retro_get_system_av_info. */
#define DOTLUA_SAMPLE_RATE 44100

/* Примитивы */
void dotlua_line(int x0, int y0, int x1, int y1, int color);
void dotlua_rect(int x, int y, int w, int h, int color);
void dotlua_oval(int x, int y, int w, int h, int color);