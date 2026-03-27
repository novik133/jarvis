# Static Bundling Implementation

## Overview
J.A.R.V.I.S. plasmoid now automatically builds and bundles llama.cpp from source during the package build process, along with all dependencies.

## What's Included
- **llama-server**: Automatically compiled from source during build
- **whisper.cpp**: Static library for speech recognition  
- **Piper TTS**: Optional download for high-quality voice synthesis

## Key Features Implemented

### 1. Automatic LLM Engine Build
- llama.cpp is automatically cloned and compiled during package build
- Built from source at version b8533 (latest stable)
- Configured for CPU-only operation with static linking where possible
- Bundled directly in the package at `/usr/libexec/jarvis/llama-server`

### 2. Streaming TTS for Faster Response
- Voice synthesis starts as soon as first complete sentence is generated
- No need to wait for full response before speaking
- Implements sentence-by-sentence streaming for natural conversation flow

### 3. Model Management
- Model list shows only downloaded models
- Delete functionality for models, voices, and Whisper models
- All models stored in user's home directory under `~/.local/share/jarvis/`

## Build Process

### Automatic During Package Build
When anyone runs `makepkg`, the build process automatically:
1. Clones llama.cpp from GitHub
2. Configures it with optimal settings
3. Compiles llama-server binary
4. Bundles it in the package

No manual steps required - everything is automated!

### Build Configuration
The llama.cpp build uses these settings:
- Static linking where possible
- CPU-only inference (no GPU dependencies)
- Release optimization
- No SSL in server
- All examples and tests disabled

## Performance Benefits
1. **Faster Startup**: Minimal dependency resolution needed
2. **Quicker Voice Response**: Streaming TTS starts speaking immediately
3. **Reliability**: Consistent build environment across systems
4. **Portability**: Works on any Arch Linux system with standard libraries

## Directory Structure
```
/usr/libexec/jarvis/llama-server    # Auto-built LLM server binary
/usr/lib/qt6/qml/org/kde/plasma/jarvis/libjarvis_plugin.so  # Plugin
~/.local/share/jarvis/              # User data directory
├── models/                         # Downloaded LLM models
├── piper-voices/                   # Downloaded TTS voices
└── whisper-models/                 # Downloaded Whisper models
```

## Dependencies
The bundled llama-server has minimal external dependencies:
- libssl.so.3, libcrypto.so.3 (standard system libraries)
- libstdc++.so.6, libgcc_s.so.1 (C++ runtime)
- libc.so.6, libm.so.6 (standard C library)

All of these are standard on any Arch Linux system.

## For Developers
See [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md) for detailed build instructions and customization options.
