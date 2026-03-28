#!/bin/bash
# Script to download pre-built llama-server for Linux x86_64

set -e

VERSION="b3961"
URL="https://github.com/ggerganov/llama.cpp/releases/download/${VERSION}/llama-${VERSION}-bin-ubuntu-x64.zip"
DESTDIR="${1:-/usr/libexec/jarvis}"

echo "Downloading llama-server ${VERSION}..."

# Create temp directory
TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

# Download and extract
cd "$TMPDIR"
curl -L -o llama.zip "$URL" 2>/dev/null || wget -q -O llama.zip "$URL" 2>/dev/null || {
    echo "Failed to download llama-server"
    exit 1
}

unzip -q llama.zip

# Install llama-server
mkdir -p "$DESTDIR"
cp build/bin/llama-server "$DESTDIR/" || cp llama-server "$DESTDIR/" 2>/dev/null || {
    echo "llama-server binary not found in archive"
    ls -la
    exit 1
}

chmod +x "$DESTDIR/llama-server"
echo "llama-server installed to $DESTDIR/llama-server"
