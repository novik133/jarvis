#!/bin/bash
set -e

# Build llama.cpp statically
LLAMA_DIR="/tmp/llama.cpp"
LLAMA_VERSION="b8533"

# Clean up any existing build
rm -rf "$LLAMA_DIR"

# Clone llama.cpp
git clone --depth 1 --branch "$LLAMA_VERSION" https://github.com/ggml-org/llama.cpp.git "$LLAMA_DIR"
cd "$LLAMA_DIR"

# Configure for static build
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
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON

# Build llama-server
cmake --build build --target llama-server -j$(nproc)

# Copy the binary to the expected location
sudo mkdir -p /usr/libexec/jarvis
sudo cp build/bin/llama-server /usr/libexec/jarvis/
sudo chmod +x /usr/libexec/jarvis/llama-server

echo "llama-server built and installed to /usr/libexec/jarvis/llama-server"

# Test if it works
if /usr/libexec/jarvis/llama-server --help > /dev/null 2>&1; then
    echo "llama-server is working correctly"
else
    echo "llama-server test failed"
    exit 1
fi
