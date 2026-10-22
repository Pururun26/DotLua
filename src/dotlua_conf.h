#pragma once

#define DOTLUA_WINDOW_WIDTH  384
#define DOTLUA_WINDOW_HEIGHT 384

/* --- Виртуальный экран (framebuffer) --- */
#define DOTLUA_WIDTH   96
#define DOTLUA_HEIGHT  96
#define DOTLUA_TITLE   "DotLua"
#define DOTLUA_FPS     30

/* Палитра */
#define DOTLUA_COLOR_COUNT       2
#define DOTLUA_DEFAULT_COLOR_0   0x72A488FF   /* фон           #72a488 */
#define DOTLUA_DEFAULT_COLOR_1   0x212C28FF   /* передний план #212c28 */

/* Максимальный размер картриджа в байтах. Больше — не запускаемся. */
#define DOTLUA_MAX_SCRIPT_SIZE 65536

/* Спрайты: 8x8, 256 штук. Значение пикселя: 0, 1 (индексы палитры)
 * или DOTLUA_TRANSPARENT (не рисовать). */
#define DOTLUA_NUM_SPRITES  256
#define DOTLUA_SPRITE_SIZE  8
#define DOTLUA_TRANSPARENT  255