#!/usr/bin/env bash
set -euo pipefail

: "${EMSDK:?Set EMSDK to your activated Emscripten SDK directory}"
source "$EMSDK/emsdk_env.sh" >/dev/null

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SEAL_SRC="${SEAL_SRC:-$ROOT/vendor/SEAL}"
BUILD="$ROOT/build/seal-wasm-intrin"

emcmake cmake -S "$SEAL_SRC" -B "$BUILD" \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS=-DEMSCRIPTEN \
  -DCMAKE_C_FLAGS=-DEMSCRIPTEN \
  -DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG -O3" \
  -DCMAKE_C_FLAGS_RELEASE="-DNDEBUG -O3" \
  -DSEAL_BUILD_BENCH=OFF \
  -DSEAL_BUILD_EXAMPLES=OFF \
  -DSEAL_BUILD_TESTS=OFF \
  -DSEAL_USE_CXX17=ON \
  -DSEAL_USE_INTRIN=ON \
  -DSEAL_USE_MSGSL=OFF \
  -DSEAL_USE_ZLIB=OFF \
  -DSEAL_USE_ZSTD=OFF \
  -DSEAL_THROW_ON_TRANSPARENT_CIPHERTEXT=ON

cmake --build "$BUILD" --parallel
mkdir -p "$ROOT/public/seal"
em++ "$ROOT/src/seal_demo.cpp" "$BUILD/lib/libseal.a" \
  -DEMSCRIPTEN \
  -I "$SEAL_SRC/native/src" -I "$BUILD/native/src" \
  --bind -O3 -flto \
  -s WASM=1 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=createSealModule \
  -s ENVIRONMENT=web \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s DISABLE_EXCEPTION_CATCHING=0 \
  -s ASSERTIONS=0 \
  -o "$ROOT/public/seal/seal.js"
