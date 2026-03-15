#!/usr/bin/env bash
set -e

OUT="teleos"
SRC="src/main.cpp src/lexer.cpp src/parser.cpp src/interpreter.cpp"
FLAGS="-O3 -march=native -std=c++17 -flto -funroll-loops -ffast-math"
INCLUDES="-Isrc"

echo "[Teleos Build] Compiling..."

g++ $FLAGS $INCLUDES $SRC -o $OUT

echo "[Teleos Build] Success! Output: ./$OUT"
echo ""
echo "Usage:   ./teleos <script.tsl>"
echo "Example: ./teleos examples/fulldemo.tsl"
