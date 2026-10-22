#include "dotlua_log.h"
#include "raylib.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifndef __EMSCRIPTEN__
static FILE *g_log_file = NULL;
#endif

static const char *level_name(int level) {
    switch (level) {
        case LOG_TRACE:   return "TRACE";
        case LOG_DEBUG:   return "DEBUG";
        case LOG_INFO:    return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR:   return "ERROR";
        case LOG_FATAL:   return "FATAL";
        default:          return "INFO";
    }
}

/* Перехват логов raylib (TraceLog). */
static void log_callback(int level, const char *text, va_list args) {
#ifdef __EMSCRIPTEN__
    char buf[1024];
    vsnprintf(buf, sizeof(buf), text, args);

    int lvl = level;
    EM_ASM({
        var lvl = $0;
        var msg = UTF8ToString($1);
        // 4 = LOG_WARNING, 5 = LOG_ERROR, 6 = LOG_FATAL (см. raylib.h)
        if (lvl === 4 || lvl === 5 || lvl === 6) {
            console.warn('[DotLua] ' + msg);
        } else {
            console.log('[DotLua] ' + msg);
        }
    }, lvl, buf);
#else
    if (!g_log_file) return;
    fprintf(g_log_file, "[%s] ", level_name(level));
    vfprintf(g_log_file, text, args);
    fputc('\n', g_log_file);
    fflush(g_log_file);
#endif
}

void dotlua_log_init(void) {
#ifdef __EMSCRIPTEN__
    EM_ASM({ console.log('=== DotLua web ==='); });
    SetTraceLogCallback(log_callback);
#else
    char path[1024];
    snprintf(path, sizeof(path), "%sdotlua_log.txt", GetApplicationDirectory());

    g_log_file = fopen(path, "w");
    if (!g_log_file) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(g_log_file, "=== DotLua %04d-%02d-%02d %02d:%02d:%02d ===\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);
    fflush(g_log_file);

    SetTraceLogCallback(log_callback);
#endif
}

void dotlua_log_close(void) {
#ifdef __EMSCRIPTEN__
    EM_ASM({ console.log('=== DotLua web log closed ==='); });
#else
    if (!g_log_file) return;
    fprintf(g_log_file, "=== log closed ===\n");
    fclose(g_log_file);
    g_log_file = NULL;
#endif
}

void dotlua_log(const char *format, ...) {
    va_list args;
    va_start(args, format);

#ifdef __EMSCRIPTEN__
    char buf[1024];
    vsnprintf(buf, sizeof(buf), format, args);
    EM_ASM({
        console.log('[DotLua] ' + UTF8ToString($0));
    }, buf);
#else
    if (g_log_file) {
        vfprintf(g_log_file, format, args);
        fputc('\n', g_log_file);
        fflush(g_log_file);
    }
#endif

    va_end(args);
}