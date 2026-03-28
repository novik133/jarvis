#!/bin/bash
set -e

# Build llama.cpp statically and copy to source for bundling
LLAMA_DIR="/tmp/llama.cpp-static"
LLAMA_VERSION="b8533"
OUTPUT_DIR="/data/jarvis/llama-bin"

# Clean up any existing build
rm -rf "$LLAMA_DIR" "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# Clone llama.cpp
git clone --depth 1 --branch "$LLAMA_VERSION" https://github.com/ggml-org/llama.cpp.git "$LLAMA_DIR"
cd "$LLAMA_DIR"

# Configure for fully static build
cmake -B build -S . \
    -DLLAMA_BUILD_EXAMPLES=OFF \
    -DLLAMA_BUILD_TESTS=OFF \
    -DLLAMA_BUILD_SERVER=ON \
    -DLLAMA_SERVER_SSL=OFF \
    -DGGML_NATIVE=OFF \
    -DGGML_CUDA=OFF \
    -DGGML_VULKAN=OFF \
    -DGGML_METAL=OFF \
    -DGGML_OPENMP=OFF \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" \
    -DCMAKE_FIND_LIBRARY_SUFFIXES=".a"

# Build llama-server statically
cmake --build build --target llama-server -j$(nproc)

# Copy the static binary to source directory
cp build/bin/llama-server "$OUTPUT_DIR/"
chmod +x "$OUTPUT_DIR/llama-server"

# Verify it's static
if ldd "$OUTPUT_DIR/llama-server" | grep -q "not found"; then
    echo "Warning: llama-server still has dynamic dependencies"
else
    echo "llama-server appears to be statically linked"
fi

echo "Static llama-server copied to $OUTPUT_DIR/llama-server"
