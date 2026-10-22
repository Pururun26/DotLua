# DotLua

A tiny fantasy console for Lua: 96×96, 2 colors, 64 KB per cartridge.

- One `.dlua` file — the entire game (code + sprites + data)
- Lua 5.4.9, without `io`/`os`/`package`/`debug`
- 2-color palette, 8×8 sprites, single-channel sound

**The API is stable** — function names, signatures, and behavior won't change. Only cosmetic additions (new functions, examples, docs) may appear.

## Build

```bash
cmake -S . -B build-native
cmake --build build-native
```

Dependencies: CMake, MSYS2 UCRT64 gcc, `raylib-6.0_win64_mingw-w64`, `lua-5.4.9`.

## Run

```bash
./run.sh examples/snake.dlua
```

`run.sh` packs the `.dlua` via `pack.py` and launches `build-native/dotlua.exe`.

## Web

```bash
source /c/emsdk/emsdk_env.sh
emcmake cmake -S . -B build-web
cmake --build build-web
```

Deploy 4 files together: `index.html`, `index.js`, `index.wasm`, `cart.dlua`. Test locally over HTTP (not `file://`):

```bash
cd build-web && python -m http.server 8000
```

## Cartridge

A `.dlua` file is a regular Lua script with a `.dlua` extension. Limit — **65536 characters** (code + sprites combined).

```lua
--! dfc 1

function update()
    cls(0)
    -- your game
end
```

Top-level code runs once at load. `update()` is called 30 times per second.

## Modules

Multi-file project: `main.dlua` + `.lua` modules alongside.

```lua
-- main.dlua
--[[utils.lua]]
--[[player.lua]]

function update()
    player_update()
end
```

`pack.py` bundles everything into a single file:
```bash
python tools/pack.py examples/my_game/main.dlua -o build-native/run.dlua
```

## Controls

| i | Button | Keys |
|---|---|---|
| 0 | LEFT | ← / A |
| 1 | RIGHT | → / D |
| 2 | UP | ↑ / W |
| 3 | DOWN | ↓ / S |
| 4 | A | Z / J |
| 5 | B | X / K |

F11 — fullscreen. Ctrl+P — FPS overlay.

## API

### Constants

| | |
|---|---|
| `96` | screen width and height |
| `30` | FPS |
| color `0` | background `#72a488` |
| color `1` | foreground `#212c28` |

### Graphics

```lua
cls(color)                -- fill screen with color (0 or 1)
pset(x, y, color)         -- set pixel (color = 0|1, default 1)
pget(x, y)                -- read pixel, returns 0 or 1
line(x0, y0, x1, y1, color)     -- line
rect(x, y, w, h, color)         -- filled rectangle
oval(x, y, w, h, color)         -- filled ellipse (w==h = circle)
print(s, x, y, color)           -- text, 3×4 font, UPPERCASE ONLY
```

- `color` — always 0 or 1 (default 1).
- `print` — lowercase is auto-converted to uppercase. No wrapping: long text runs off the edge and gets clipped. `\n` — explicit line break.

### Sprites

```lua
sprdata(id, art)          -- load an 8×8 sprite into slot id (0..255)
spr(id, x, y)             -- draw sprite
```

`art` — 8×8 string:

| Char | Value |
|---|---|
| `0` | color 0 |
| `1` or `#` | color 1 |
| `3`, `.`, space | transparent |

```lua
sprdata(0, [[
0##00##0
#00##00#
#000000#
0#0000#0
00#00#00
000##000
00000000
00000000
]])
spr(0, 44, 44)
```

- Smaller than 8×8 — the rest is transparent.
- Larger than 8×8 — the excess is discarded. For 16×16 — compose from 4 sprites.
- Unknown character — `error()` (typos are caught immediately).

### Input

```lua
btn(i)   -- true while button is held (0..5)
btnp(i)  -- true only on the frame of press (0..5)
```

### Sound

```lua
beep(freq, dur, vol)      -- tone at freq Hz, dur frames, vol 0..100
```

- Single channel (monophonic).
- `dur` in frames: `15` = 0.5 s, `30` = 1 s (at 30 FPS).
- `beep(0, 0, 0)` — silence.
- A new `beep` interrupts the current one.

### Time

```lua
time()    -- milliseconds since game start
utime()   -- Unix timestamp in seconds
```

`time()` for animation, timers, cooldowns. `utime()` for `math.randomseed(utime())` and persistent games.

### Save

```lua
dset(slot, value)     -- write int64 to slot (0..63)
dget(slot)            -- read int64 from slot (0..63)
```

- **64 slots × 8 bytes = 512 bytes** per cartridge.
- `dset` writes immediately (crash-safe).
- Desktop: `save/<cartname>.bin` next to the exe.
- Web: `localStorage['storage']` (persists across reloads).

```lua
high = dget(0)
if score > high then
    high = score
    dset(0, high)
end
```

### Standard Lua

`math`, `string`, `table`, `utf8`, `coroutine` are available. **No** `io`, `os`, `package`, `debug`, `loadfile`, `dofile`. `load` (for strings) is available.

## Example: mini-game

```lua
math.randomseed(utime())

px, py = 48, 48
food = { x = math.random(0, 11), y = math.random(0, 11) }
score = 0

function update()
    if btn(0) then px = px - 1 end
    if btn(1) then px = px + 1 end
    if btn(2) then py = py - 1 end
    if btn(3) then py = py + 1 end

    if math.floor(px / 8) == food.x and math.floor(py / 8) == food.y then
        food = { x = math.random(0, 11), y = math.random(0, 11) }
        score = score + 1
        beep(880, 5)
    end

    cls(0)
    rect(px - 2, py - 2, 4, 4, 1)
    rect(food.x * 8, food.y * 8, 8, 8, 1)
    print("SCORE:" .. score, 0, 0)
end
```

## Limitations

- 96×96, 2 colors
- 65536 characters per cartridge (code + sprites)
- 256 sprites, 8×8 each
- 64 save slots × 8 bytes
- No `require` — modules via `--[[file.lua]]`
- No `io`/`os`/`package`/`debug` — sandboxed
- No network, no file I/O (except cartridge loading)

## Project layout

```
src/
  dotlua.c             — main, window, loop, log
  dotlua_conf.h        — constants (WIDTH, FPS, colors)
  dotlua_api.c/.h      — C implementation of the API
  dotlua_api_lua.c/.h  — Lua bindings
  dotlua_log.c/.h      — logging (file on desktop, console.log on web)
  dotlua_save.c/.h     — dset/dget (file on desktop, localStorage on web)
  dotlua_font3x4.h     — font
web/
  shell.html           — web shell (fetch cart.dlua, canvas CSS)
tools/
  pack.py              — builds .dlua from main + modules
examples/
  snake.dlua           — snake
  ...                  — other examples
```

## Build outputs

- `build-native/dotlua.exe` — desktop binary
- `build-native/dotlua_log.txt` — log of the last run
- `build-native/save/<name>.bin` — save data
- `build-web/index.html` + `.js` + `.wasm` — web build