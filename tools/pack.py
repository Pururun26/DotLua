#!/usr/bin/env python3
"""
pack.py — DotLua packer.

Собирает .dlua картридж с --[[file.lua]] включениями в один файл.
Алгоритм идентичен C-препроцессору (dotlua_preproc.c).

Usage:
    python tools/pack.py path/to/main.dlua [-o out.dlua]
    python tools/pack.py path/to/main.dlua
        -> path/to/main.packed.dlua
"""

import argparse
import os
import re
import sys

MAX_DEPTH   = 16
INCLUDE_RE  = re.compile(r'^--\s*\[\[(.+\.lua)\]\]$')


def parse_include(line):
    m = INCLUDE_RE.match(line.strip())
    return m.group(1) if m else None


def pack_file(path, depth, out):
    if depth > MAX_DEPTH:
        raise RuntimeError(f"include depth exceeded ({MAX_DEPTH}): {path}")

    base_dir = os.path.dirname(path) or "."
    fname    = os.path.basename(path)

    try:
        with open(path, "r", encoding="utf-8", newline="") as f:
            lines = f.readlines()
    except FileNotFoundError:
        raise RuntimeError(f"cannot open '{path}'")
    except OSError as e:
        raise RuntimeError(f"cannot read '{path}': {e}")

    if depth > 0:
        out.append(f"-- >>> {fname}")

    for raw in lines:
        line = raw.rstrip("\r\n")
        name = parse_include(line)
        if name is not None:
            pack_file(os.path.join(base_dir, name), depth + 1, out)
        else:
            out.append(line)

    if depth > 0:
        out.append(f"-- <<< {fname}")


def main():
    ap = argparse.ArgumentParser(description="DotLua packer")
    ap.add_argument("cart", help="path to main .dlua")
    ap.add_argument("-o", "--output", help="output .dlua (default: <cart>.packed.dlua)")
    args = ap.parse_args()

    out = []
    try:
        pack_file(args.cart, 0, out)
    except RuntimeError as e:
        print(f"pack.py: {e}", file=sys.stderr)
        return 1

    out_path = args.output
    if not out_path:
        root, ext = os.path.splitext(args.cart)
        out_path = root + ".packed" + (ext or ".dlua")

    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        for line in out:
            f.write(line + "\n")

    print(f"packed {args.cart} -> {out_path} ({len(out)} lines)")
    return 0


if __name__ == "__main__":
    sys.exit(main())