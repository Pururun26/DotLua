#include "dotlua_api_lua.h"
#include "dotlua_api.h"
#include "lauxlib.h"
#include "lualib.h"

static int l_cls(lua_State *L) {
    dotlua_cls((int)luaL_optinteger(L, 1, 0));
    return 0;
}

static int l_pset(lua_State *L) {
    dotlua_pset((int)luaL_checkinteger(L, 1),
                (int)luaL_checkinteger(L, 2),
                (int)luaL_optinteger  (L, 3, 1));
    return 0;
}

static int l_pget(lua_State *L) {
    lua_pushinteger(L, dotlua_pget(
        (int)luaL_checkinteger(L, 1),
        (int)luaL_checkinteger(L, 2)));
    return 1;
}

static int l_print(lua_State *L) {
    const char *s = luaL_checkstring(L, 1);
    int x = (int)luaL_optinteger(L, 2, 0);
    int y = (int)luaL_optinteger(L, 3, 0);
    int c = (int)luaL_optinteger(L, 4, 1);
    dotlua_print(s, x, y, c);
    return 0;
}


static int l_sprdata(lua_State *L) {
    int id = (int)luaL_checkinteger(L, 1);
    const char *art = luaL_checkstring(L, 2);
    dotlua_sprdata(id, art);
    return 0;
}

static int l_spr(lua_State *L) {
    dotlua_spr(
        (int)luaL_checkinteger(L, 1),
        (int)luaL_checkinteger(L, 2),
        (int)luaL_checkinteger(L, 3));
    return 0;
}


static int l_btn(lua_State *L) {
    int i = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, dotlua_btn(i));
    return 1;
}

static int l_btnp(lua_State *L) {
    int i = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, dotlua_btnp(i));
    return 1;
}


static int l_time(lua_State *L) {
    lua_pushinteger(L, (lua_Integer)dotlua_time());
    return 1;
}

static int l_utime(lua_State *L) {
    lua_pushinteger(L, (lua_Integer)dotlua_utime());
    return 1;
}


static int l_dset(lua_State *L) {
    int slot = (int)luaL_checkinteger(L, 1);
    lua_Integer v = luaL_checkinteger(L, 2);
    dotlua_dset(slot, (int64_t)v);
    return 0;
}

static int l_dget(lua_State *L) {
    int slot = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, (lua_Integer)dotlua_dget(slot));
    return 1;
}


static int l_beep(lua_State *L) {
    int freq = (int)luaL_checkinteger(L, 1);
    int dur  = (int)luaL_checkinteger(L, 2);
    int vol  = (int)luaL_optinteger(L, 3, 100);
    dotlua_beep(freq, dur, vol);
    return 0;
}


static int l_line(lua_State *L) {
    dotlua_line(
        (int)luaL_checkinteger(L, 1), (int)luaL_checkinteger(L, 2),
        (int)luaL_checkinteger(L, 3), (int)luaL_checkinteger(L, 4),
        (int)luaL_optinteger  (L, 5, 1));
    return 0;
}

static int l_rect(lua_State *L) {
    dotlua_rect(
        (int)luaL_checkinteger(L, 1), (int)luaL_checkinteger(L, 2),
        (int)luaL_checkinteger(L, 3), (int)luaL_checkinteger(L, 4),
        (int)luaL_optinteger  (L, 5, 1));
    return 0;
}

static int l_oval(lua_State *L) {
    dotlua_oval(
        (int)luaL_checkinteger(L, 1), (int)luaL_checkinteger(L, 2),
        (int)luaL_checkinteger(L, 3), (int)luaL_checkinteger(L, 4),
        (int)luaL_optinteger  (L, 5, 1));
    return 0;
}

static const luaL_Reg api_funcs[] = {
    {"cls",     l_cls},
    {"pset",    l_pset},
    {"pget",    l_pget},
    {"print",   l_print},
    {"line",    l_line},
    {"rect",    l_rect},
    {"oval",    l_oval},
    {"sprdata", l_sprdata},
    {"spr",     l_spr},
    {"btn",     l_btn},
    {"btnp",    l_btnp},
    {"time",    l_time},
    {"utime",   l_utime},
    {"dset",    l_dset},
    {"dget",    l_dget},
    {"beep",    l_beep},
    {NULL, NULL}
};

void dotlua_openlibs(lua_State *L) {
    luaL_requiref(L, LUA_GNAME,        luaopen_base,   1); lua_pop(L, 1);
    luaL_requiref(L, LUA_MATHLIBNAME,  luaopen_math,   1); lua_pop(L, 1);
    luaL_requiref(L, LUA_STRLIBNAME,   luaopen_string, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_TABLIBNAME,   luaopen_table,  1); lua_pop(L, 1);

    lua_pushnil(L); lua_setglobal(L, "loadfile");
    lua_pushnil(L); lua_setglobal(L, "dofile");
}

void dotlua_register_api(lua_State *L) {
    lua_pushglobaltable(L);
    luaL_setfuncs(L, api_funcs, 0);
    lua_pop(L, 1);
}