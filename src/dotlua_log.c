#include "dotlua_log.h"
#include "raylib.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static FILE *g_log_file = NULL;

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
    if (!g_log_file) return;
    fprintf(g_log_file, "[%s] ", level_name(level));
    vfprintf(g_log_file, text, args);
    fputc('\n', g_log_file);
    fflush(g_log_file);
}

void dotlua_log_init(void) {
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
}

void dotlua_log_close(void) {
    if (!g_log_file) return;
    fprintf(g_log_file, "=== log closed ===\n");
    fclose(g_log_file);
    g_log_file = NULL;
}

void dotlua_log(const char *format, ...) {
    if (!g_log_file) return;
    va_list args;
    va_start(args, format);
    vfprintf(g_log_file, format, args);
    fputc('\n', g_log_file);
    fflush(g_log_file);
    va_end(args);
}