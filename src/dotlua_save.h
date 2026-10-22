#pragma once
#include <stddef.h>
#include <stdint.h>

/* 64 слота по 8 байт (int64_t) = 512 байт.
 * Файл: <application_dir>/save/<cartname>.bin */

void dotlua_save_open(const char *cart_path);   /* создать save/, загрузить данные */
void dotlua_save_close(void);                   /* финальный flush */

void    dotlua_dset(int slot, int64_t value);
int64_t dotlua_dget(int slot);