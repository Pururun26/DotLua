#!/usr/bin/env bash
set -e

CART="${1:-examples/my_game/main.dlua}"
OUT="build/run.dlua"

if [ ! -f "$CART" ]; then
    echo "run.sh: cart not found: $CART" >&2
    echo "usage: ./run.sh [path/to/main.dlua]" >&2
    exit 1
fi

python tools/pack.py "$CART" -o "$OUT"
./build/dotlua.exe "$OUT"