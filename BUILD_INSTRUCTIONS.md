# Build Instructions for J.A.R.V.I.S. KDE Plasmoid

## Overview
This document explains how to build J.A.R.V.I.S. KDE Plasmoid from source. The build process automatically compiles and bundles llama.cpp, so no external dependencies need to be installed manually.

## System Requirements
- Arch Linux (or compatible distribution)
- CMake 3.20+
- GCC 11+ or Clang 12+
- Qt6 development libraries
- KDE Frameworks 6 development libraries
- Git
- Make/Ninja

## Arch Linux Dependencies
```bash
sudo pacman -S base-devel cmake git extra-cmake-modules qt6-base qt6-declarative qt6-multimedia qt6-speech kdeclarative ki18n kconfig kcoreaddons plasma-workspace
```

## Build Process

### 1. Clone the Repository
```bash
git clone https://github.com/novik133/jarvis.git
cd jarvis
```

### 2. Build the Package
The build process automatically:
- Downloads and compiles llama.cpp from source
- Builds whisper.cpp as a static library
- Compiles the J.A.R.V.I.S. plugin
- Bundles everything into a package

```bash
makepkg -f
```

### 3. Install the Package
```bash
sudo pacman -U jarvis-plasmoid-*.pkg.tar.zst
```

## What Gets Built

### llama.cpp
- **Source**: Automatically cloned from https://github.com/ggml-org/llama.cpp.git
- **Version**: b8533 (latest stable)
- **Configuration**: 
  - Static linking where possible
  - CPU-only (no GPU support)
  - No SSL in server
  - Optimized for release build
- **Output**: `/usr/libexec/jarvis/llama-server`

### whisper.cpp
- **Source**: Bundled as FetchContent dependency
- **Build Type**: Static library
- **Features**: CPU-only inference

### J.A.R.V.I.S. Plugin
- **Location**: `/usr/lib/qt6/qml/org/kde/plasma/jarvis/libjarvis_plugin.so`
- **Dependencies**: All bundled, no external requirements

## Build Customization

### Changing llama.cpp Version
Edit `CMakeLists.txt` and modify the `GIT_TAG` value:
```cmake
GIT_TAG        <desired-tag-or-commit>
```

### Enabling GPU Support
Add GPU flags to the CMake configuration in `cmake/BuildLlama.cmake.in`:
```cmake
-DGGML_CUDA=ON  # For NVIDIA GPUs
# or
-DGGML_VULKAN=ON  # For Vulkan-compatible GPUs
```

### Build Directory
The build process creates temporary directories:
- `build/llama-src` - llama.cpp source code
- `build/llama-build` - llama.cpp build files
- `build/llama-server` - final binary

## Troubleshooting

### Build Fails with GCC 15
If you encounter build errors with GCC 15, the build script automatically suppresses strict error flags.

### Out of Memory During Build
llama.cpp requires significant RAM to build. Close other applications or increase swap space if needed.

### Missing Dependencies
Ensure all required packages are installed. The build system will report any missing dependencies.

## Package Contents
After installation, the package includes:
- `/usr/libexec/jarvis/llama-server` - LLM inference server
- `/usr/lib/qt6/qml/org/kde/plasma/jarvis/` - KDE plasmoid plugin
- `/usr/share/plasma/plasmoids/org.kde.plasma.jarvis/` - Plasmoid QML/UI files
- Static libraries for whisper.cpp

## Runtime Requirements
After installation, J.A.R.V.I.S. requires:
- Models to be downloaded through the settings UI
- Optional: Piper TTS (downloaded on demand)
- Standard system libraries (automatically satisfied on Arch Linux)

## Development
For development or debugging, you can build without packaging:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

The binaries will be in the `build/` directory.
