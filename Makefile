# DotLua — UCRT64 (MSYS2)
# Артефакты сборки: build/

CC      := gcc
AR      := ar
CFLAGS  := -std=c99 -Wall -Wextra -O2
CFLAGS  += -Isrc
CFLAGS  += -Ilua-5.4.9
CFLAGS  += -Iraylib/raylib-6.0_win64_mingw-w64/include

LDFLAGS := -Lraylib/raylib-6.0_win64_mingw-w64/lib \
           -static -static-libgcc -static-libstdc++ \
		   -mwindows

# -l:libraylib.a — форсируем статическую raylib (не libraylib.dll.a!)
LIBS    := -l:libraylib.a -lopengl32 -lgdi32 -lwinmm -lshell32 -lpthread

# --- директории ---
BUILD   := build
OBJDIR  := $(BUILD)/obj
LIBDIR  := $(BUILD)/lib

# --- Lua: режем опасное ---
# Исключены: liolib, loslib, loadlib, ldblib, linit
LUA_SRC := \
    lua-5.4.9/lapi.c \
    lua-5.4.9/lauxlib.c \
    lua-5.4.9/lbaselib.c \
    lua-5.4.9/lcode.c \
    lua-5.4.9/lcorolib.c \
    lua-5.4.9/lctype.c \
    lua-5.4.9/ldebug.c \
    lua-5.4.9/ldo.c \
    lua-5.4.9/ldump.c \
    lua-5.4.9/lfunc.c \
    lua-5.4.9/lgc.c \
    lua-5.4.9/llex.c \
    lua-5.4.9/lmathlib.c \
    lua-5.4.9/lmem.c \
    lua-5.4.9/lobject.c \
    lua-5.4.9/lopcodes.c \
    lua-5.4.9/lparser.c \
    lua-5.4.9/lstate.c \
    lua-5.4.9/lstring.c \
    lua-5.4.9/lstrlib.c \
    lua-5.4.9/ltable.c \
    lua-5.4.9/ltablib.c \
    lua-5.4.9/ltm.c \
    lua-5.4.9/lundump.c \
    lua-5.4.9/lutf8lib.c \
    lua-5.4.9/lvm.c \
    lua-5.4.9/lzio.c

LUA_OBJ := $(patsubst lua-5.4.9/%.c,$(OBJDIR)/lua/%.o,$(LUA_SRC))
LUA_LIB := $(LIBDIR)/liblua_dotlua.a

# --- DotLua ---
APP_SRC := src/dotlua.c src/dotlua_api.c src/dotlua_api_lua.c src/dotlua_log.c
APP_OBJ := $(patsubst src/%.c,$(OBJDIR)/app/%.o,$(APP_SRC))

TARGET  := $(BUILD)/dotlua.exe

# --- правила ---
all: $(TARGET)

# 1) Lua как статическая библиотека
$(LUA_LIB): $(LUA_OBJ) | $(LIBDIR)
	$(AR) rcs $@ $^

# 2) DotLua
$(TARGET): $(APP_OBJ) $(LUA_LIB) | $(BUILD)
	$(CC) $(APP_OBJ) $(LUA_LIB) -o $@ $(LDFLAGS) $(LIBS)

$(OBJDIR)/lua/%.o: lua-5.4.9/%.c | $(OBJDIR)/lua
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/app/%.o: src/%.c | $(OBJDIR)/app
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD) $(LIBDIR) $(OBJDIR)/lua $(OBJDIR)/app:
	mkdir -p $@

run: $(TARGET)
	@CART="$(CART)"; CART="$${CART:-examples/my_game/main.dlua}"; \
	python tools/pack.py "$$CART" -o build/run.dlua && \
	./build/dotlua.exe build/run.dlua

clean:
	rm -rf $(BUILD)

.PHONY: all run clean