#!/bin/bash

set -e

CC=${CC:-gcc}
CFLAGS="${CFLAGS:--Wall -Wextra -O2 -std=c99}"
LDFLAGS="${LDFLAGS:--static}"

SRC_DIR="src"
EXAMPLE_DIR="example"
BUILD_DIR="build"

# Create build directory
mkdir -p "$BUILD_DIR"

echo "Building dogarm library..."

# Compile all source files
$CC $CFLAGS -c "$SRC_DIR/operand.c" -o "$BUILD_DIR/operand.o"
$CC $CFLAGS -c "$SRC_DIR/decode.c" -o "$BUILD_DIR/decode.o"
$CC $CFLAGS -c "$SRC_DIR/disasm.c" -o "$BUILD_DIR/disasm.o"
$CC $CFLAGS -c "$SRC_DIR/dogarm.c" -o "$BUILD_DIR/dogarm.o"

# Create static library
ar rcs "$BUILD_DIR/libdogarm.a" "$BUILD_DIR/operand.o" "$BUILD_DIR/decode.o" "$BUILD_DIR/disasm.o" "$BUILD_DIR/dogarm.o"

# Create shared library
echo "Creating shared library..."
$CC $CFLAGS -shared -fPIC "$SRC_DIR/operand.c" "$SRC_DIR/decode.c" "$SRC_DIR/disasm.c" "$SRC_DIR/dogarm.c" -o "$BUILD_DIR/libdogarm.so"

echo "Building example..."

# Compile example (link statically)
$CC $CFLAGS -I. "$EXAMPLE_DIR/main.c" -L"$BUILD_DIR" -ldogarm -static -o "$BUILD_DIR/example"

echo "Building tools..."

# Compile dogarm-disasm tool
mkdir -p "$BUILD_DIR/tools"
$CC $CFLAGS -I. "tools/dogarm-disasm.c" -L"$BUILD_DIR" -ldogarm -static -o "$BUILD_DIR/dogarm-disasm"

echo "Build complete!"
echo "Run the example with: ./$BUILD_DIR/example"
echo "Disassemble a file with: ./$BUILD_DIR/dogarm-disasm <file>"

