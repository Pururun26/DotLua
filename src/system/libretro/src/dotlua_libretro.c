#include "libretro.h"
#include "video.h"
#include "dotlua_api.h"
#include "dotlua_api_lua.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>

static retro_environment_t        environ_cb;
static retro_video_refresh_t      video_cb;
static retro_audio_sample_t       audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t         input_poll_cb;
static retro_input_state_t        input_state_cb;

static lua_State *g_L     = NULL;

static const void *g_cart_data = NULL;
static size_t      g_cart_size = 0;
static bool        g_cart_loaded = false;

/* --- Лог через RetroArch --- */
static void retro_log(enum retro_log_level level, const char *fmt, ...) {
    char buf[1024];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    struct retro_log_callback log_cb;
    if (environ_cb && environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log_cb)) {
        log_cb.log(level, "%s", buf);
    } else {
        fprintf(stderr, "%s\n", buf);
    }
}

static int64_t now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000 + (int64_t)ts.tv_nsec / 1000;
}

static double  g_last_audio_sec = 0.0;
static int64_t g_last_update_us = 0;
#define FRAME_INTERVAL_US      (1000000 / 30)        /* 33333 мкс */
#define FRAME_INTERVAL_MIN_US  (FRAME_INTERVAL_US * 9 / 10)   /* 30000 мкс */

RETRO_API unsigned retro_api_version(void) {
    return RETRO_API_VERSION;
}

RETRO_API void retro_set_environment(retro_environment_t cb)        { environ_cb = cb; }
RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb)    { video_cb = cb; }
RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb)      { audio_cb = cb; }
RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
RETRO_API void retro_set_input_poll(retro_input_poll_t cb)          { input_poll_cb = cb; }
RETRO_API void retro_set_input_state(retro_input_state_t cb)        { input_state_cb = cb; }

RETRO_API void retro_get_system_info(struct retro_system_info *info) {
    memset(info, 0, sizeof(*info));
    info->library_name     = "DotLua";
    info->library_version  = "1.0.1";
    info->need_fullpath    = false;
    info->valid_extensions = "dlua";
    info->block_extract    = false;
}

RETRO_API void retro_get_system_av_info(struct retro_system_av_info *info) {
    memset(info, 0, sizeof(*info));
    info->geometry.base_width   = DOTLUA_W;
    info->geometry.base_height  = DOTLUA_H;
    info->geometry.max_width    = DOTLUA_W;
    info->geometry.max_height   = DOTLUA_H;
    info->geometry.aspect_ratio = (float)DOTLUA_W / (float)DOTLUA_H;
    info->timing.fps            = 30.0;
    info->timing.sample_rate    = 44100.0;
    info->timing.sample_rate    = DOTLUA_SAMPLE_RATE;
}

RETRO_API void retro_init(void) {
    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
    environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt);
    dotlua_video_init();
}

RETRO_API void retro_deinit(void) {
    dotlua_video_deinit();
}

RETRO_API void retro_reset(void) {
    g_last_audio_sec = 0.0;
    g_last_update_us = 0;

    if (g_L) {
        lua_close(g_L);
        g_L = NULL;
    }

    if (g_cart_data && g_cart_size > 0) {
        g_L = luaL_newstate();
        if (g_L) {
            dotlua_openlibs(g_L);
            dotlua_register_api(g_L);

            if (luaL_loadbuffer(g_L, g_cart_data, g_cart_size, "cart") != LUA_OK) {
                retro_log(RETRO_LOG_ERROR, "DotLua: reset load error: %s",
                          lua_tostring(g_L, -1));
                lua_pop(g_L, 1);
                lua_close(g_L);
                g_L = NULL;
            } else if (lua_pcall(g_L, 0, 0, 0) != LUA_OK) {
                retro_log(RETRO_LOG_ERROR, "DotLua: reset runtime error: %s",
                          lua_tostring(g_L, -1));
                lua_pop(g_L, 1);
                lua_close(g_L);
                g_L = NULL;
            } else {
                dotlua_time_reset();
                retro_log(RETRO_LOG_INFO, "DotLua: cartridge reset");
            }
        }
    }
    g_cart_loaded = (g_L != NULL);
}

RETRO_API bool retro_load_game(const struct retro_game_info *game) {
    if (!game || !game->data || game->size == 0) {
        retro_log(RETRO_LOG_ERROR, "DotLua: empty cartridge");
        return false;
    }

    g_last_audio_sec = 0.0;
    g_last_update_us = 0;

    g_cart_data   = game->data;
    g_cart_size   = game->size;
    g_cart_loaded = false;

    g_L = luaL_newstate();
    if (!g_L) {
        retro_log(RETRO_LOG_ERROR, "DotLua: luaL_newstate failed");
        return false;
    }

    dotlua_openlibs(g_L);
    dotlua_register_api(g_L);

    retro_log(RETRO_LOG_INFO, "DotLua: cartridge prepared (%zu bytes)", g_cart_size);
    return true;
}

RETRO_API void retro_unload_game(void) {
    if (g_L) {
        lua_close(g_L);
        g_L = NULL;
    }
    g_cart_data   = NULL;
    g_cart_size   = 0;
    g_cart_loaded = false;
}

/* Читает 6 кнопок с порта 0 и упаковывает в маску. */
static int read_buttons(void) {
    if (!input_state_cb) return 0;

    int mask = 0;
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))  mask |= 1 << 0;
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT)) mask |= 1 << 1;
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))    mask |= 1 << 2;
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))  mask |= 1 << 3;
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A))     mask |= 1 << 4;
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B))     mask |= 1 << 5;

    return mask;
}

RETRO_API void retro_run(void) {
    input_poll_cb();

    /* --- Audio: генерируем по реальному времени, независимо от частоты --- */
    if (audio_batch_cb) {
        double now = (double)now_us() / 1000000.0;
        if (g_last_audio_sec == 0.0) g_last_audio_sec = now;

        double dt = now - g_last_audio_sec;
        g_last_audio_sec = now;
        if (dt > 0.1) dt = 0.1;
        if (dt < 0.0) dt = 0.0;

        int frames = (int)(dt * DOTLUA_SAMPLE_RATE);
        if (frames > 4096) frames = 4096;

        if (frames > 0) {
            static int16_t buf[4096 * 2];
            dotlua_audio_render(buf, frames);
            audio_batch_cb(buf, (size_t)frames);
        }
    }

    /* --- Отложенная загрузка картриджа (после того, как RetroArch подложил SRAM) --- */
    if (g_L && !g_cart_loaded) {
        g_cart_loaded = true; /* чтобы не пытаться снова при ошибке */

        if (luaL_loadbuffer(g_L, g_cart_data, g_cart_size, "cart") != LUA_OK) {
            retro_log(RETRO_LOG_ERROR, "DotLua: load error: %s", lua_tostring(g_L, -1));
            lua_pop(g_L, 1);
            lua_close(g_L);
            g_L = NULL;
        } else if (lua_pcall(g_L, 0, 0, 0) != LUA_OK) {
            retro_log(RETRO_LOG_ERROR, "DotLua: runtime error: %s", lua_tostring(g_L, -1));
            lua_pop(g_L, 1);
            lua_close(g_L);
            g_L = NULL;
        } else {
            dotlua_time_reset(); /* time() = 0 в первом update() */
            retro_log(RETRO_LOG_INFO, "DotLua: cartridge loaded");
        }
    }

    /* --- Game logic: троттлинг 30 FPS --- */
    int64_t now = now_us();
    if (now - g_last_update_us >= FRAME_INTERVAL_MIN_US) {
        g_last_update_us = now;

        dotlua_btn_update(read_buttons());

        if (g_L) {
            lua_getglobal(g_L, "update");
            if (lua_isfunction(g_L, -1)) {
                if (lua_pcall(g_L, 0, 0, 0) != LUA_OK) {
                    retro_log(RETRO_LOG_ERROR, "DotLua: update error: %s",
                              lua_tostring(g_L, -1));
                    lua_pop(g_L, 1);
                }
            } else {
                lua_pop(g_L, 1);
            }
        }
    }

    video_cb(dotlua_video_get_framebuffer(),
             dotlua_video_get_width(),
             dotlua_video_get_height(),
             dotlua_video_get_pitch());
}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device) {
    (void)port; (void)device;
}

RETRO_API void retro_cheat_reset(void) {}
RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code) {
    (void)index; (void)enabled; (void)code;
}

RETRO_API size_t retro_serialize_size(void) { return 0; }
RETRO_API bool   retro_serialize(void *data, size_t size)         { (void)data; (void)size; return false; }
RETRO_API bool   retro_unserialize(const void *data, size_t size) { (void)data; (void)size; return false; }

RETRO_API bool retro_load_game_special(unsigned type,
                                       const struct retro_game_info *info,
                                       size_t num) {
    (void)type; (void)info; (void)num;
    return false;
}

RETRO_API unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }

RETRO_API void *retro_get_memory_data(unsigned id) {
    if (id == RETRO_MEMORY_SAVE_RAM) return dotlua_save_data();
    return NULL;
}

RETRO_API size_t retro_get_memory_size(unsigned id) {
    if (id == RETRO_MEMORY_SAVE_RAM) return dotlua_save_size();
    return 0;
}