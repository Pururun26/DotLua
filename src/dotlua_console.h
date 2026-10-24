#pragma once

// ============================================================
// Консоль на Windows для GUI-приложений (-mwindows).
// Позволяет printf работать, если dotlua.exe запущен из cmd.
// На Linux / macOS / Web — пустышки.
// ============================================================

#ifdef _WIN32

// Отсекаем куски windows.h, конфликтующие с raylib:
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#define NOMINMAX

#include <windows.h>
#include <stdio.h>

static void dotlua_console_attach(void) {
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        freopen("CONIN$",  "r", stdin);
    }
}

static void dotlua_console_detach(void) {
    FreeConsole();
}

#else
static void dotlua_console_attach(void) { }
static void dotlua_console_detach(void) { }
#endif